# ══════════════════════════════════════════════════
#  Makefile — TCP 원격 장치 제어 프로젝트
#
#  RPi4 타겟 (aarch64) : webserver, lib*.so
#  Ubuntu 타겟 (native): webclient
#
#  사용법:
#    make          전체 빌드
#    make libs     공유 라이브러리만
#    make send     instructor 서버로 전송
#    make clean    빌드 파일 삭제
# ══════════════════════════════════════════════════

CC_CROSS  = aarch64-linux-gnu-gcc
CC_HOST   = gcc

CROSS_INC = -I/usr/aarch64-linux-gnu/include
CROSS_LIB = -L/usr/aarch64-linux-gnu/lib

WIRING    = -lwiringPi -lcrypt
CROSS_FLAGS = -Wall -fPIC $(CROSS_INC)

.PHONY: all libs send clean

all: webclient webserver libs send

libs: libled.so libsensor.so libsgmt.so libsound.so

# ── 클라이언트 (Ubuntu native) ──────────────────
webclient: webclient.c
	$(CC_HOST) -Wall -o $@ $<

# ── 서버 (RPi4 aarch64) ─────────────────────────
webserver: webserver.c
	$(CC_CROSS) $(CROSS_FLAGS) -o $@ $< \
	    $(CROSS_LIB) $(WIRING) -lpthread -ldl

# ── 공유 라이브러리 (RPi4 aarch64) ─────────────
libled.so: led.c
	$(CC_CROSS) $(CROSS_FLAGS) -shared -o $@ $< \
	    $(CROSS_LIB) $(WIRING)

libsensor.so: sensor.c
	$(CC_CROSS) $(CROSS_FLAGS) -shared -o $@ $< \
	    $(CROSS_LIB) $(WIRING) -lm

libsgmt.so: sgmt.c
	$(CC_CROSS) $(CROSS_FLAGS) -shared -o $@ $< \
	    $(CROSS_LIB) $(WIRING)

libsound.so: sound.c
	$(CC_CROSS) $(CROSS_FLAGS) -shared -o $@ $< \
	    $(CROSS_LIB) $(WIRING)

# ── 전송 (instructor@100.126.193.83) ─────────────
send: libs
	~/send.sh

# ── 정리 ─────────────────────────────────────────
clean:
	rm -f webclient webserver libled.so libsensor.so libsgmt.so libsound.so
