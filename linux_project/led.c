/*
 * led.c — LED 제어 공유 라이브러리 (libled.so)
 *
 * 내보내는 심볼:
 *   void led_on(void)           LED 켜기
 *   void led_off(void)          LED 끄기
 *   void led_brightness(int lv) 밝기 설정 (1=30%, 2=60%, 3=100%)
 */
#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>

#define LED 28

void led_on(void)
{
    wiringPiSetup();
    pinMode(LED, OUTPUT);
    softPwmCreate(LED, 0,255);
    
    //digitalWrite(LED, HIGH);
    softPwmWrite(LED,250);
    printf("[LED] ON\n");
    fflush(stdout);
}

void led_off(void)
{
    wiringPiSetup();
    pinMode(LED, OUTPUT);
    softPwmCreate(LED, 0,255);
    //digitalWrite(LED, LOW);
    softPwmWrite(LED,0);
    printf("[LED] OFF\n");
    fflush(stdout);
}

void led_brightness(int level)
{
    wiringPiSetup();
     pinMode(LED,OUTPUT);
    softPwmCreate(LED, 0,255);

    int duty = 0;
    if      (level == 1) duty = 50;
    else if (level == 2) duty = 130;
    else if (level == 3) duty = 250;
    softPwmWrite(LED,duty);
    printf("[LED] 밝기 %d단계 (%d%%)\n", level, duty);
    fflush(stdout);
}