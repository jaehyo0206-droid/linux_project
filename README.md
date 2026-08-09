# Linux Project

C와 Linux 시스템 프로그래밍을 기반으로 Raspberry Pi의 하드웨어를 제어하고, 네트워크를 통해 상태를 확인할 수 있도록 구현한 임베디드 Linux 프로젝트입니다.

Raspberry Pi에서 GPIO와 I2C를 이용해 LED, 조도 센서, 온도 센서, 7-Segment, 부저 등의 하드웨어를 제어하고, 각 기능을 공유 라이브러리(`.so`)로 분리했습니다.

또한 TCP 기반의 Web Server / Web Client를 구현하여 하드웨어 제어 기능을 네트워크로 확장했습니다.

---

## 프로젝트 개요

### 개발 목적

Linux 환경에서 C 언어를 이용하여 하드웨어를 직접 제어하고, 이를 라이브러리와 네트워크 프로그램으로 확장하면서 임베디드 Linux 시스템의 구조와 동작 원리를 학습하는 것을 목표로 했습니다.

주요 구현 내용은 다음과 같습니다.

* Raspberry Pi GPIO 제어
* WiringPi 기반 하드웨어 제어
* I2C 기반 센서 데이터 수집
* LED 밝기 제어
* 조도에 따른 LED 자동 제어
* 온도 센서 데이터 계산
* 7-Segment 제어
* 부저 제어
* 기능별 공유 라이브러리(`.so`) 제작
* TCP Web Server 구현
* TCP Web Client 구현
* Makefile을 이용한 빌드 자동화

---

## 개발 환경

| 구분            | 내용                    |
| ------------- | --------------------- |
| Language      | C                     |
| OS            | Linux                 |
| Hardware      | Raspberry Pi          |
| GPIO Library  | WiringPi              |
| Communication | I2C / TCP             |
| Build         | Makefile              |
| Library       | Shared Object (`.so`) |
| Compiler      | GCC                   |

---

## 프로젝트 구조

```text
linux_project/
└── linux_project/
    ├── Makefile
    │
    ├── led.c
    ├── sensor.c
    ├── sgmt.c
    ├── sound.c
    │
    ├── webserver.c
    ├── webclient.c
    │
    ├── libled.so
    ├── libsensor.so
    ├── libsgmt.so
    ├── libsound.so
    │
    ├── webserver
    └── webclient
```

### 주요 파일

| 파일             | 설명                    |
| -------------- | --------------------- |
| `led.c`        | LED GPIO 제어           |
| `sensor.c`     | 조도 및 온도 센서 제어         |
| `sgmt.c`       | 7-Segment 제어          |
| `sound.c`      | 부저 제어                 |
| `webserver.c`  | TCP 서버 구현             |
| `webclient.c`  | TCP 클라이언트 구현          |
| `Makefile`     | 전체 프로젝트 빌드 관리         |
| `libled.so`    | LED 제어 공유 라이브러리       |
| `libsensor.so` | 센서 제어 공유 라이브러리        |
| `libsgmt.so`   | 7-Segment 제어 공유 라이브러리 |
| `libsound.so`  | 부저 제어 공유 라이브러리        |

---

## 주요 기능

### 1. LED 제어

`led.c`에서 Raspberry Pi GPIO를 이용한 LED 제어 기능을 구현했습니다.

WiringPi와 `softPwm`을 이용하여 LED의 ON/OFF뿐만 아니라 밝기를 단계별로 조절할 수 있도록 구현했습니다.

```c
void led_on(void);
void led_off(void);
void led_brightness(int level);
```

LED 밝기 제어는 PWM Duty 값을 변경하는 방식으로 구현했습니다.

```text
Level 1 → 낮은 밝기
Level 2 → 중간 밝기
Level 3 → 높은 밝기
```

---

## 2. 센서 제어

`sensor.c`에서는 Raspberry Pi의 I2C 인터페이스를 이용하여 PCF8591 ADC 기반 센서 데이터를 읽습니다.

### 사용 센서

* 조도 센서(CDS)
* NTC 온도 센서

PCF8591의 I2C 주소를 기반으로 `/dev/i2c-1` 인터페이스를 통해 센서 데이터를 읽도록 구현했습니다.

---

## 3. 조도 센서

조도 센서의 ADC 값을 읽어 주변 밝기를 확인합니다.

```c
int cds_read(void);
```

ADC 값은 `0 ~ 255` 범위로 처리합니다.

또한 설정한 기준값을 기준으로 LED를 자동 제어할 수 있도록 구현했습니다.

```c
void cds_auto_led(int threshold);
```

동작 과정:

```text
조도 센서
    ↓
ADC 값 측정
    ↓
Threshold 비교
    ↓
LED 자동 ON / OFF
```

---

## 4. 온도 측정

NTC 써미스터의 ADC 값을 이용하여 온도를 계산합니다.

```c
float temp_read(void);
```

전체적인 처리 과정은 다음과 같습니다.

```text
NTC 센서
   ↓
PCF8591 ADC
   ↓
ADC 값
   ↓
NTC 저항값 계산
   ↓
온도 계산
   ↓
섭씨 온도 출력
```

---

## 5. 센서 데이터 통합

여러 센서의 데이터를 하나의 결과로 구성할 수 있도록 통합 함수를 구현했습니다.

```c
void sensor_read_all(char *buf, int len);
```

조도 및 온도 데이터를 하나의 문자열로 구성하여 애플리케이션에서 사용할 수 있도록 했습니다.

---

## 6. 7-Segment 제어

`sgmt.c`에서는 Raspberry Pi GPIO를 이용하여 7-Segment를 제어합니다.

센서 값이나 상태 정보를 숫자로 표시할 수 있도록 구성했습니다.

```text
Raspberry Pi
     │
     └── GPIO
          │
          ↓
      7-Segment
          │
          ↓
       숫자 표시
```

---

## 7. 부저 제어

`sound.c`에서는 Raspberry Pi GPIO를 이용하여 부저를 제어합니다.

부저 제어 기능을 별도의 소스 파일로 분리하여 다른 애플리케이션에서도 사용할 수 있도록 구성했습니다.

---

## 8. 공유 라이브러리

각 하드웨어 제어 기능을 공유 라이브러리로 분리했습니다.

```text
led.c
   ↓
libled.so

sensor.c
   ↓
libsensor.so

sgmt.c
   ↓
libsgmt.so

sound.c
   ↓
libsound.so
```

### 구조

```text
┌──────────────────────────┐
│       Application        │
│  webserver / webclient   │
└────────────┬─────────────┘
             │
             ↓
┌──────────────────────────┐
│     Shared Libraries     │
│                          │
│ libled.so                │
│ libsensor.so             │
│ libsgmt.so               │
│ libsound.so              │
└────────────┬─────────────┘
             │
             ↓
┌──────────────────────────┐
│ Raspberry Pi Hardware    │
│ GPIO / I2C               │
└──────────────────────────┘
```

하드웨어 제어 코드를 애플리케이션과 분리하여 코드 재사용성과 모듈화를 높였습니다.

---

## 9. TCP Web Server

`webserver.c`에서는 Linux Socket API를 이용하여 TCP 기반 서버를 구현했습니다.

클라이언트의 요청을 받아 Raspberry Pi의 하드웨어 제어 기능과 연결할 수 있도록 구성했습니다.

```text
Client
   │
   │ TCP
   ↓
Web Server
   │
   ├── LED 제어
   ├── Sensor 조회
   ├── 7-Segment
   └── Sound
        │
        ↓
Raspberry Pi Hardware
```

---

## 10. TCP Web Client

`webclient.c`에서는 TCP Socket을 이용하여 서버와 통신하는 클라이언트를 구현했습니다.

전체 통신 구조는 다음과 같습니다.

```text
┌──────────────┐
│ Web Client   │
└──────┬───────┘
       │
       │ TCP Socket
       ↓
┌──────────────┐
│ Web Server   │
└──────┬───────┘
       │
       ↓
┌────────────────────┐
│ Raspberry Pi       │
│                    │
│ GPIO / I2C         │
│ LED / Sensor       │
│ 7-Segment / Sound  │
└────────────────────┘
```

이를 통해 Linux Socket Programming과 네트워크 기반 임베디드 제어를 함께 구현했습니다.

---

## 11. Makefile

프로젝트 빌드를 자동화하기 위해 `Makefile`을 사용했습니다.

소스 코드와 공유 라이브러리를 각각 빌드하고 최종 실행 파일을 생성할 수 있도록 구성했습니다.

```text
Source Code
     │
     ↓
   GCC
     │
     ├── libled.so
     ├── libsensor.so
     ├── libsgmt.so
     ├── libsound.so
     │
     ├── webserver
     └── webclient
```

---

## 전체 시스템 구조

```text
                         TCP
┌──────────────┐  ──────────────────>  ┌──────────────┐
│ Web Client   │                         │ Web Server   │
└──────────────┘  <──────────────────   └──────┬───────┘
                                               │
                                               ↓
                                    ┌─────────────────────┐
                                    │ Shared Libraries    │
                                    │                     │
                                    │ libled.so           │
                                    │ libsensor.so        │
                                    │ libsgmt.so           │
                                    │ libsound.so         │
                                    └──────────┬──────────┘
                                               │
                         ┌─────────────────────┼──────────────────┐
                         ↓                     ↓                  ↓
                       GPIO                   I2C               GPIO
                         ↓                     ↓                  ↓
                       LED              PCF8591 ADC         7-Segment
                                             │
                                      ┌──────┴──────┐
                                      ↓             ↓
                                   조도 센서      온도 센서
```

---

## 기술적으로 경험한 내용

### Linux 시스템 프로그래밍

Linux 환경에서 C 언어를 사용하여 하드웨어 및 네트워크 프로그램을 구현했습니다.

* Linux 환경에서 C 프로그램 개발
* GCC 컴파일 및 링크
* GPIO 제어
* I2C 통신
* Socket Programming
* Client / Server 구조 구현
* Shared Library 제작 및 활용
* Makefile 기반 빌드 자동화

### 하드웨어 제어

Raspberry Pi의 GPIO 및 I2C 인터페이스를 직접 사용하여 주변장치를 제어했습니다.

```text
GPIO
 ├── LED
 ├── 7-Segment
 └── Buzzer

I2C
 └── PCF8591
      ├── CDS
      └── NTC
```

### 공유 라이브러리 설계

하드웨어 제어 기능을 각각의 `.so` 파일로 분리하여 애플리케이션과 하드웨어 제어 계층을 분리했습니다.

이를 통해 코드 재사용성과 모듈화에 대한 경험을 쌓았습니다.

### Socket Programming

TCP Socket을 이용하여 Client와 Server 간 통신을 구현했습니다.

```text
Socket
  ↓
Bind
  ↓
Listen
  ↓
Accept
  ↓
Receive / Send
  ↓
Hardware Control
```

네트워크 프로그램과 임베디드 하드웨어를 연결하는 구조를 직접 구현했습니다.

---

## 프로젝트를 통해 얻은 경험

* C 기반 Linux 시스템 프로그래밍 경험
* Raspberry Pi GPIO 제어 경험
* I2C 통신 및 ADC 센서 데이터 처리 경험
* 센서 데이터를 이용한 하드웨어 제어 경험
* Shared Library(`.so`) 제작 및 활용 경험
* TCP Socket Programming 경험
* Client / Server 구조 이해
* Makefile 기반 빌드 자동화 경험
* 하드웨어 계층과 애플리케이션 계층을 분리하는 모듈화 경험

---

## 향후 개선 방향

* Socket 통신 프로토콜 개선
* 멀티 클라이언트 지원
* `select()` / `poll()` / `epoll()` 기반 I/O Multiplexing
* pthread를 이용한 멀티스레드 서버
* 하드웨어 제어 API 추상화
* 센서 데이터 실시간 모니터링
* 웹 기반 GUI 추가
* MQTT 기반 IoT 통신 연동
* Docker 기반 실행 환경 구성
* Linux Device Driver 직접 구현
* 시스템 로그 및 에러 처리 강화

---

## 프로젝트 의의

이 프로젝트를 통해 단순한 C 프로그램 작성에서 벗어나 다음과 같은 임베디드 Linux 시스템의 구조를 직접 구현했습니다.

```text
C
 ↓
Linux
 ↓
GPIO / I2C
 ↓
Hardware Control
 ↓
Shared Library
 ↓
Socket Programming
 ↓
Client / Server
```

Raspberry Pi의 하드웨어를 Linux 프로그램에서 직접 제어하고, 이를 공유 라이브러리와 TCP 네트워크 프로그램으로 확장하면서 시스템 프로그래밍과 임베디드 개발에 필요한 저수준 제어 및 소프트웨어 구조화 경험을 쌓았습니다.
