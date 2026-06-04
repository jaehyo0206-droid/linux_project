/*
 * sgmt.c — 7세그먼트 공유 라이브러리 (libsgmt.so)
 *
 * 내보내는 심볼:
 *   void seven_segment_control(int start)
 *       start(0~9)에서 0까지 1초 간격으로 카운트다운 표시
 */
#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h>

/* wiringPi 핀 번호 (BCD 입력 핀) */
static int gpiopins[4] = {4, 1, 16, 15};

/* 0~9 BCD 코드 */
static int bcd[10][4] = {
    {0,0,0,0}, /* 0 */
    {0,0,0,1}, /* 1 */
    {0,0,1,0}, /* 2 */
    {0,0,1,1}, /* 3 */
    {0,1,0,0}, /* 4 */
    {0,1,0,1}, /* 5 */
    {0,1,1,0}, /* 6 */
    {0,1,1,1}, /* 7 */
    {1,0,0,0}, /* 8 */
    {1,0,0,1}  /* 9 */
};

static void fnd_show(int num)
{
    int i;
    for (i = 0; i < 4; i++)
        pinMode(gpiopins[i], OUTPUT);
    for (i = 0; i < 4; i++)
        digitalWrite(gpiopins[i], bcd[num][i] ? HIGH : LOW);

    printf("[7세그] %d 표시\n", num);
    fflush(stdout);
    sleep(1);

    /* 표시 끄기 */
    for (i = 0; i < 4; i++)
        digitalWrite(gpiopins[i], HIGH);
}

void seven_segment_control(int start)
{
    if (start < 0 || start > 9) {
        printf("[7세그] 범위 오류: %d (0~9)\n", start);
        fflush(stdout);
        return;
    }

    wiringPiSetup();
    printf("[7세그] 카운트다운 %d → 0 시작\n", start);
    fflush(stdout);

    int i;
    for (i = start; i >= 0; i--)
        fnd_show(i);

    printf("[7세그] 카운트다운 완료\n");
    fflush(stdout);
}
