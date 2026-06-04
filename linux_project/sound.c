/*
 * sound.c — 부저(buzzer) 공유 라이브러리 (libsound.so)
 *
 * 내보내는 심볼:
 *   void musicPlay(void)   도레미송 연주
 *   void beep(void)        단음 비프 (카운트다운 완료 알림용)
 */
#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>
#include <unistd.h>

#define SPKR   6
#define TOTAL  32

static int notes[] = {
    391, 391, 440, 440, 391, 391, 330, 330,
    391, 391, 330, 330, 264, 264, 294, 0,
    391, 391, 440, 440, 391, 391, 330, 330,
    391, 393, 294, 330, 262, 262, 262, 0
};

void musicPlay(void)
{
    printf("change");
    wiringPiSetup();
    softToneCreate(SPKR);
    printf("[부저] 음악 재생 시작\n");
    fflush(stdout);

    int i;
    for (i = 0; i < TOTAL; i++) {
        softToneWrite(SPKR, notes[i]);
        delay(280);
    }
    softToneWrite(SPKR, 0);
    printf("[부저] 음악 재생 완료\n");
    fflush(stdout);
}

/*
void beep(void)
{
    wiringPiSetup();
    softToneCreate(SPKR);
    printf("[부저] 카운트다운 완료 알림\n");
    fflush(stdout);

    softToneWrite(SPKR, 880);
    delay(500);
    softToneWrite(SPKR, 0);
}
*/