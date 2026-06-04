/*
 * sensor.c — YL-40(PCF8591) I2C ADC 공유 라이브러리 (libsensor.so)
 *
 * 내보내는 심볼:
 *   int   cds_read(void)                       조도 ADC 값(0~255), 실패 시 -1
 *   void  cds_auto_led(int threshold)           조도 기준 LED 자동 제어
 *   float temp_read(void)                       NTC 온도(°C), 실패 시 -999.0
 *   void  sensor_read_all(char *buf, int len)   조도+온도 읽기+LED 제어+결과 문자열
 */
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define PCF8591_ADDR  0x48   /* YL-40 I2C 주소 */
#define CDS_CHANNEL   0x00   /* AIN0: 조도센서 */
#define TEMP_CHANNEL  0x01   /* AIN1: NTC 써미스터 */
#define LED_PIN       28

/* NTC 써미스터 파라미터 (YL-40 기본 부품 기준) */
#define NTC_R0     10000.0f  /* 25°C 기준 저항 10kΩ */
#define NTC_B      3950.0f   /* B 상수 */
#define NTC_T0     298.15f   /* 기준 온도 25°C → Kelvin */
#define R_FIXED    10000.0f  /* 분압 저항 10kΩ */

#define CDS_ADDR PCF8591_ADDR  /* 하위 호환 */

int cds_read(void)
{
    wiringPiSetup();

    int fd = wiringPiI2CSetupInterface("/dev/i2c-1", PCF8591_ADDR);
    if (fd < 0) {
        fprintf(stderr, "[조도센서] I2C 초기화 실패\n");
        fflush(stderr);
        return -1;
    }

    wiringPiI2CWrite(fd, CDS_CHANNEL);
    wiringPiI2CRead(fd);               /* 이전 값(garbage) 버리기 */
    int val = wiringPiI2CRead(fd);
    close(fd);

    printf("[조도센서] ADC=%d\n", val);
    fflush(stdout);
    return val;
}

float temp_read(void)
{
    wiringPiSetup();

    int fd = wiringPiI2CSetupInterface("/dev/i2c-1", PCF8591_ADDR);
    if (fd < 0) {
        fprintf(stderr, "[온도센서] I2C 초기화 실패\n");
        fflush(stderr);
        return -999.0f;
    }

    wiringPiI2CWrite(fd, TEMP_CHANNEL);
    wiringPiI2CRead(fd);                  /* 이전 값(garbage) 버리기 */
    int adc = wiringPiI2CRead(fd);
    close(fd);

    if (adc <= 0 || adc >= 255) {
        fprintf(stderr, "[온도센서] ADC 범위 오류: %d\n", adc);
        fflush(stderr);
        return -999.0f;
    }

    /* 분압 회로: Vcc — R_FIXED — AIN1 — NTC — GND
     * R_ntc = R_FIXED * adc / (255 - adc)
     * Steinhart-Hart B 파라미터 식:
     *   1/T = 1/T0 + (1/B) * ln(R_ntc / R0) */
    float r_ntc = R_FIXED * (float)adc / (255.0f - (float)adc);
    float temp_k = 1.0f / (1.0f / NTC_T0 + logf(r_ntc / NTC_R0) / NTC_B);
    float temp_c = temp_k - 273.15f;

    printf("[온도센서] ADC=%d, R=%.1f Ω, T=%.1f°C\n", adc, r_ntc, temp_c);
    fflush(stdout);
    return temp_c;
}

void cds_auto_led(int threshold)
{
    int val = cds_read();
    if (val < 0) return;

    wiringPiSetup();
    pinMode(LED_PIN, OUTPUT);

    if (val < threshold) {
        digitalWrite(LED_PIN, HIGH);
        printf("[조도센서] 밝음(%d < %d) → LED OFF\n", val, threshold);
    } else {
        digitalWrite(LED_PIN, LOW);
        printf("[조도센서] 어두움(%d >= %d) → LED ON\n", val, threshold);
    }
    fflush(stdout);
}

#define CDS_THRESHOLD 180

void sensor_read_all(char *buf, int len)
{
    int   cds  = cds_read();
    float temp = temp_read();

    /* 조도 기준 LED 자동 제어 */
    wiringPiSetup();
    pinMode(LED_PIN, OUTPUT);
    if (cds >= 0)
        digitalWrite(LED_PIN, cds < CDS_THRESHOLD ? HIGH : LOW);

    /* 결과 문자열 조합 */
    char cds_str[64], temp_str[32];

    if (cds < 0)
        snprintf(cds_str, sizeof(cds_str), "조도: 읽기 실패");
    else
        snprintf(cds_str, sizeof(cds_str), "조도: %d → %s",
                 cds, cds < CDS_THRESHOLD ? "밝음 (LED ON)" : "어두움 (LED OFF)");

    if (temp <= -999.0f)
        snprintf(temp_str, sizeof(temp_str), "온도: 읽기 실패");
    else
        snprintf(temp_str, sizeof(temp_str), "온도: %.1f°C", temp);

    snprintf(buf, len, "%s | %s", cds_str, temp_str);

    printf("[센서] %s\n", buf);
    fflush(stdout);
}
