/*
 * webclient.c — Ubuntu 클라이언트
 *
 * 빌드: gcc -o webclient webclient.c
 * 실행: ./webclient <RPi4-IP> <port>
 *
 * 시그널:
 *   SIGINT  → 종료 (g_running 플래그)
 *   그 외   → 무시 (SIG_IGN)
 */
#define _POSIX_C_SOURCE 200809L
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

static int make_connection(const char *ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "잘못된 IP: %s\n", ip);
        close(sockfd);
        exit(1);
    }
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(1);
    }
    return sockfd;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    /* ── 시그널 처리 ──
     * 소프트웨어 시그널은 모두 무시하고,
     * SIGINT(Ctrl+C)만 g_running 플래그를 내려 정상 종료한다.
     * sigaction + SA_RESTART: 다른 시그널이 시스템 콜을 끊지 않도록 보장.
     * SIGINT는 SA_RESTART 없이 등록해 scanf/recv를 즉시 중단시킨다. */
    static const int ignore_sigs[] = {
        SIGTERM, SIGHUP, SIGPIPE, SIGQUIT,
        SIGUSR1, SIGUSR2, SIGALRM, SIGVTALRM, SIGPROF, SIGTSTP
    };
    struct sigaction sa_ign = { .sa_handler = SIG_IGN };
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = SA_RESTART;
    for (size_t i = 0; i < sizeof(ignore_sigs)/sizeof(ignore_sigs[0]); i++)
        sigaction(ignore_sigs[i], &sa_ign, NULL);

    struct sigaction sa_int = { .sa_handler = sigint_handler };
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;   /* SA_RESTART 없음: scanf/recv를 EINTR로 깨움 */
    sigaction(SIGINT, &sa_int, NULL);

    while (g_running) {
        int sockfd = make_connection(ip, port);

        /* 서버 메뉴 수신 */
        char buf[1024];
        int n = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { perror("recv menu"); close(sockfd); break; }
        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);

        /* 메뉴 선택 입력 */
        int menu;
        if (scanf("%d", &menu) != 1) {
            close(sockfd);
            if (!g_running) break;          /* SIGINT로 인한 EOF */
            printf("입력 종료\n");
            break;
        }
        if (!g_running) { close(sockfd); break; }

        if (menu == 0) {
            printf("종료합니다.\n");
            close(sockfd);
            break;
        }

        /* 메뉴 번호 전송 */
        send(sockfd, &menu, sizeof(menu), 0);

        /* ── 메뉴별 추가 입력 ── */
        if (menu == 1) {
            /* 7세그먼트: 시작 숫자 */
            int num;
            while (1) {
                printf("카운트다운 시작 숫자 (0~9): ");
                fflush(stdout);
                if (scanf("%d", &num) != 1) {
                    int c; while ((c = getchar()) != '\n' && c != EOF);
                    printf("숫자를 입력하세요.\n");
                    continue;
                }
                if (num < 0 || num > 9) {
                    printf("0~9 범위의 숫자만 입력 가능합니다.\n");
                    continue;
                }
                break;
            }
            send(sockfd, &num, sizeof(num), 0);
            printf("→ 숫자 %d 전송\n", num);

        } else if (menu == 2) {
            /* LED 제어 서브메뉴 */
            printf("\n=== LED 제어 ===\n");
            printf("1. LED ON\n");
            printf("2. LED OFF\n");
            printf("3. 밝기 1단계 (낮음 30%%)\n");
            printf("4. 밝기 2단계 (중간 60%%)\n");
            printf("5. 밝기 3단계 (높음 100%%)\n");
            printf("선택: ");
            fflush(stdout);

            int subcmd;
            if (scanf("%d", &subcmd) != 1) {
                printf("입력 오류\n");
                close(sockfd);
                continue;
            }
            send(sockfd, &subcmd, sizeof(subcmd), 0);
        }
        /* 메뉴 3(부저), 4(조도센서)는 추가 입력 없음 */

        /* 서버 응답 수신 */
        n = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            printf("[서버] %s\n\n", buf);
        }

        close(sockfd);
    }

    return 0;
}
