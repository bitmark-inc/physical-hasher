# Copyright Bitmark Inc 2015-2015

VIDEO_DEVICE ?= /dev/video1
FIRMWARE ?= ../bitmark-microscope/bitmark-microscope.img
DOWNLOAD_FX3 = ../../../../cyusb_linux_1.0.4/src/download_fx3

FRAMES ?= 10
BRIGHTNESS ?= 200
SHARPNESS ?= 0x00f0
CONTRAST ?= 18
FRAMES_OUT ?= frames.data

CAPTURE_OPTS = -c "${FRAMES}" -t
CAPTURE_OPTS += -o "${FRAMES_OUT}"
CAPTURE_OPTS += -d "${VIDEO_DEVICE}"
CAPTURE_OPTS += --brightness="${BRIGHTNESS}"
CAPTURE_OPTS += --sharpness="${SHARPNESS}"
CAPTURE_OPTS += --contrast="${CONTRAST}"

CFLAGS += -std=gnu99 -Wall -Werror

LFLAGS += -lbsd

.PHONY: all
all: capture create-png list-controls test-leds

capture: capture.c
	${CC} ${CFLAGS} -o "$@" "$<" ${LFLAGS}

create-png: create-png.c
	${CC} ${CFLAGS}  -o "$@" "$<" $$(pkg-config --cflags --libs libpng12)

list-controls: list-controls.c
	${CC} ${CFLAGS} -o "$@" "$<" ${LFLAGS}


test-leds: test-leds.c
	${CC} ${CFLAGS} -o "$@" "$<" ${LFLAGS}


.PHONY: led
led: test-leds
	-[ ! -e "${VIDEO_DEVICE}" ] && $(MAKE) download
	[ -e "${VIDEO_DEVICE}" ] && ./test-leds -d "${VIDEO_DEVICE}" -c 100

.PHONY: lc
lc: list-controls
	-[ ! -e "${VIDEO_DEVICE}" ] && $(MAKE) download
	[ -e "${VIDEO_DEVICE}" ] && ./list-controls -d "${VIDEO_DEVICE}"


.PHONY: cap
cap: capture
	-[ ! -e "${VIDEO_DEVICE}" ] && $(MAKE) download
	[ -e "${VIDEO_DEVICE}" ] && ./capture ${CAPTURE_OPTS}

.PHONY: dec
dec: create-png
	./create-png

.PHONY: download
download:
	$(DOWNLOAD_FX3) -t RAM -i "$(FIRMWARE)"
	@for i in 1 2 3 4 5 6 7 8 9 10; \
	 do \
	   [ -e "${VIDEO_DEVICE}" ] && break; \
	   printf '.'; \
	   sleep 1; \
	done; \
	sleep 1; \
	printf '.\n'

.PHONY: cheese
cheese: all
	-[ ! -e "${VIDEO_DEVICE}" ] && $(MAKE) download
	[ -e "${VIDEO_DEVICE}" ] && cheese -d "${VIDEO_DEVICE}"
