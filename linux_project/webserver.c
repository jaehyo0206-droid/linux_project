/*
 * webserver.c — RPi4 TCP 서버 (데몬 프로세스)
 *
 * 빌드:
 *   aarch64-linux-gnu-gcc -o webserver webserver.c \
 *       -lpthread -ldl -lwiringPi -lcrypt
 *
 * 실행: ./webserver <port>
 * 로그: tail -f /tmp/rpi_server.log
 *
 * 프로토콜:
 *   서버 → 클라이언트 : 메뉴 문자열
 *   클라이언트 → 서버 : int menu
 *   menu==1 → 클라이언트가 추가로 int num 전송
 *   menu==2 → 클라이언트가 추가로 int subcmd 전송
 *   서버 → 클라이언트 : 결과 문자열
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>

/* ══════════════════════════════════════
 * 데몬화
 *   workdir: .so 파일이 있는 디렉터리 (실행 시 CWD)
 * ══════════════════════════════════════ */
static void daemonize(const char *workdir)
{
    pid_t pid;

    /* 1차 fork: 터미널 세션 분리 */
    pid = fork();
    if (pid < 0) { perror("fork1"); exit(EXIT_FAILURE); }
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) { perror("setsid"); exit(EXIT_FAILURE); }

    /* 2차 fork: 터미널 재획득 방지 */
    pid = fork();
    if (pid < 0) { perror("fork2"); exit(EXIT_FAILURE); }
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir(workdir);   /* .so 파일 경로 유지 */

    /* stdin → /dev/null, stdout/stderr → 로그 파일 */
    int fd = open("/dev/null", O_RDONLY);
    dup2(fd, STDIN_FILENO);
    close(fd);

    fd = open("/tmp/rpi_server.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    printf("=== 데몬 시작 (PID=%d) ===\n", getpid());
    fflush(stdout);
}

/* ══════════════════════════════════════
 * 부저 호출 헬퍼 (dlopen 방식)
 * ══════════════════════════════════════ */
static void play_music(void)
{
    void *h = dlopen("./libsound.so", RTLD_LAZY | RTLD_NODELETE);
    if (!h) { fprintf(stderr, "dlopen libsound: %s\n", dlerror()); return; }
    dlerror();
    void (*fp)(void) = dlsym(h,"musicPlay");
    if (fp) fp();
    dlclose(h);
}

/*static void play_beep(void)
{
    void *h = dlopen("./libsound.so", RTLD_LAZY | RTLD_NODELETE);
    if (!h) { fprintf(stderr, "dlopen libsound: %s\n", dlerror()); return; }
    dlerror();
    void (*fp)(void) = dlsym(h, "beep");
    if (fp) fp();
    dlclose(h);
}*/

/* ══════════════════════════════════════
 * 7세그먼트 스레드
 *   카운트다운 완료 후 자동으로 부저 울림
 * ══════════════════════════════════════ */
typedef struct { int num; } SGMT_ARG;

static void *sgmt_thread(void *arg)
{
    SGMT_ARG *a = (SGMT_ARG *)arg;
    int num = a->num;
    free(a);

    void *h = dlopen("./libsgmt.so", RTLD_LAZY | RTLD_NODELETE);
    if (!h) {
        fprintf(stderr, "dlopen libsgmt: %s\n", dlerror());
        pthread_exit(NULL);
    }
    dlerror();
    void (*fp)(int) = dlsym(h, "seven_segment_control");
    if (!fp) {
        fprintf(stderr, "dlsym seven_segment_control: %s\n", dlerror());
        dlclose(h);
        pthread_exit(NULL);
    }

    fp(num);        /* 카운트다운 (블로킹) */
    dlclose(h);

    play_music();
    //play_beep();    /* 카운트다운 완료 → 부저 */
    pthread_exit(NULL);
}

/* ══════════════════════════════════════
 * 부저 스레드 (메뉴 3 — 비동기 재생)
 * ══════════════════════════════════════ */
static void *buzzer_thread(void *arg)
{
    (void)arg;
    play_music();
    pthread_exit(NULL);
}

/* ══════════════════════════════════════
 * main
 * ══════════════════════════════════════ */
int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    /* 데몬화 전에 CWD 저장 */
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) { perror("getcwd"); return -1; }

    daemonize(cwd);

    signal(SIGPIPE, SIG_IGN);   /* broken pipe로 인한 서버 종료 방지 */

    /* 소켓 생성 */
    int ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons((uint16_t)atoi(argv[1]));

    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); return -1;
    }
    if (listen(ssock, 10) < 0) { perror("listen"); return -1; }

    printf("[서버] 대기 중 (port=%s)\n", argv[1]);
    fflush(stdout);

    while (1) {
        struct sockaddr_in cliaddr;
        unsigned int len = sizeof(cliaddr);
        int csock = accept(ssock, (struct sockaddr *)&cliaddr, &len);
        if (csock < 0) { perror("accept"); continue; }

        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, ipbuf, sizeof(ipbuf));
        printf("[접속] %s:%d\n", ipbuf, ntohs(cliaddr.sin_port));
        fflush(stdout);

        /* ── 메뉴 전송 ── */
        const char *menuMsg =
            "\n===== 원격 장치 제어 =====\n"
            "1. 7세그먼트 (카운트다운)\n"
            "2. LED 제어\n"
            "3. 부저 (음악 재생)\n"
            "4. 조도센서\n"
            "0. 종료\n"
            "선택: ";
        send(csock, menuMsg, strlen(menuMsg), 0);

        /* ── 메뉴 수신 ── */
        int menu = 0;
        if (recv(csock, &menu, sizeof(menu), 0) <= 0) { close(csock); continue; }
        printf("[메뉴] %d 선택\n", menu);
        fflush(stdout);

        char resp[256] = {0};
        pthread_t tid;

        switch (menu) {

        /* ── case 1: 7세그먼트 ── */
        case 1: {
            int num = 0;
            if (recv(csock, &num, sizeof(num), 0) <= 0) {
                snprintf(resp, sizeof(resp), "[오류] 숫자 수신 실패");
                break;
            }
            printf("[7세그] 시작 숫자=%d\n", num);
            fflush(stdout);

            SGMT_ARG *a = malloc(sizeof(SGMT_ARG));
            a->num = num;
            pthread_create(&tid, NULL, sgmt_thread, a);
            pthread_detach(tid);

            snprintf(resp, sizeof(resp), "7세그먼트: %d → 0 카운트다운 시작!", num);
            break;
        }

        /* ── case 2: LED ── */
        case 2: {
            int subcmd = 0;
            if (recv(csock, &subcmd, sizeof(subcmd), 0) <= 0) {
                snprintf(resp, sizeof(resp), "[오류] LED 명령 수신 실패");
                break;
            }
            void *h = dlopen("./libled.so", RTLD_LAZY | RTLD_NODELETE);
            if (!h) {
                snprintf(resp, sizeof(resp), "[오류] libled.so: %s", dlerror());
                break;
            }
            dlerror();
            if (subcmd == 1) {
                void (*f)(void) = dlsym(h, "led_on");
                if (f) f();
                snprintf(resp, sizeof(resp), "LED ON");
            } else if (subcmd == 2) {
                void (*f)(void) = dlsym(h, "led_off");
                if (f) f();
                snprintf(resp, sizeof(resp), "LED OFF");
            } else if (subcmd >= 3 && subcmd <= 5) {
                void (*f)(int) = dlsym(h, "led_brightness");
                int lv = subcmd - 2;   /* 3→1, 4→2, 5→3 */
                if (f) f(lv);
                snprintf(resp, sizeof(resp), "LED 밝기 %d단계 설정", lv);
            } else {
                snprintf(resp, sizeof(resp), "[오류] 잘못된 LED 명령: %d", subcmd);
            }
            dlclose(h);
            break;
        }

        /* ── case 3: 부저 ── */
        case 3:
            pthread_create(&tid, NULL, buzzer_thread, NULL);
            pthread_detach(tid);
            snprintf(resp, sizeof(resp), "부저(음악) 재생 시작");
            break;

        /* ── case 4: 조도센서 ── */
        case 4: {
            void *h = dlopen("./libsensor.so", RTLD_LAZY | RTLD_NODELETE);
            if (!h) {
                snprintf(resp, sizeof(resp), "[오류] libsensor.so: %s", dlerror());
                break;
            }
            dlerror();
            int  (*read_fn)(void) = dlsym(h, "cds_read");
            void (*auto_fn)(int)  = dlsym(h, "cds_auto_led");

            int val = (read_fn) ? read_fn() : -1;
            if (auto_fn) auto_fn(180);
            dlclose(h);

            if (val < 0)
                snprintf(resp, sizeof(resp), "[오류] 조도센서 읽기 실패");
            else
                snprintf(resp, sizeof(resp),
                         "조도값: %d → %s",
                         val, val < 180 ? "밝음 (LED ON)" : "어두움 (LED OFF)");
            break;
        }

        default:
            snprintf(resp, sizeof(resp), "[오류] 잘못된 메뉴: %d", menu);
            break;
        }

        send(csock, resp, strlen(resp), 0);
        printf("[응답] %s\n", resp);
        fflush(stdout);
        close(csock);
    }

    close(ssock);
    return 0;
}
