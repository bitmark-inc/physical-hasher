# Copyright Bitmark Inc 2015-2015

VIDEO_DEVICE ?= /dev/video1
FIRMWARE ?= ../bitmark-microscope/bitmark-microscope.img
DOWNLOAD_FX3 = ../../../../cyusb_linux_1.0.4/src/download_fx3

FRAMES ?= 300
BRIGHTNESS ?= 300
SHARPNESS ?= 0x03f0
CONTRAST ?= 21
LEDS ?= 0x0f
FRAMES_OUT ?= frames.data

ANIMATION_DELAY ?= 15
ANIMATION_SIZE ?= 640x480
ANIMATION_OFFSET ?= +0+0
ANIMATION_EXTRACT = ${ANIMATION_SIZE}${ANIMATION_OFFSET}


CAPTURE_OPTS = -c "${FRAMES}" -t
CAPTURE_OPTS += -o "${FRAMES_OUT}"
CAPTURE_OPTS += -d "${VIDEO_DEVICE}"
CAPTURE_OPTS += --brightness="${BRIGHTNESS}"
CAPTURE_OPTS += --sharpness="${SHARPNESS}"
CAPTURE_OPTS += --contrast="${CONTRAST}"
CAPTURE_OPTS += --leds="${LEDS}"

CFLAGS += -std=gnu99 -Wall -Werror

LFLAGS += -lbsd

RM = rm -f


.PHONY: all
all: capture create-png list-controls test-leds

capture: capture.c
	${CC} ${CFLAGS} -o '$@' '$<' ${LFLAGS}

create-png: create-png.c
	${CC} ${CFLAGS}  -o '$@' '$<' $$(pkg-config --cflags --libs libpng12)

list-controls: list-controls.c
	${CC} ${CFLAGS} -o '$@' '$<' ${LFLAGS}


test-leds: test-leds.c
	${CC} ${CFLAGS} -o '$@' '$<' ${LFLAGS}


.PHONY: led
led: test-leds
	-[ ! -e '${VIDEO_DEVICE}' ] && $(MAKE) download
	[ -e '${VIDEO_DEVICE}' ] && ./test-leds -d '${VIDEO_DEVICE}' -c 100

.PHONY: lc
lc: list-controls
	-[ ! -e '${VIDEO_DEVICE}' ] && $(MAKE) download
	[ -e '${VIDEO_DEVICE}' ] && ./list-controls -d '${VIDEO_DEVICE}'


.PHONY: cap
cap: capture
	-[ ! -e '${VIDEO_DEVICE}' ] && $(MAKE) download
	[ -e '${VIDEO_DEVICE}' ] && ./capture ${CAPTURE_OPTS}

.PHONY: dec
dec: create-png
	./create-png


.PHONY: full
full:
	${RM} animated_full.gif
	convert -dispose previous -delay '${ANIMATION_DELAY}' -loop 0 'frame*.png' animated_full.gif

.PHONY: small
small:
	${RM} animated_small.gif
	convert -dispose previous -delay '${ANIMATION_DELAY}' -loop 0 -page '${ANIMATION_SIZE}' -extract '${ANIMATION_EXTRACT}' 'frame*.png' animated_small.gif


.PHONY: run
run: cap dec small
	eog animated_small.gif

.PHONY: download
download:
	$(DOWNLOAD_FX3) -t RAM -i '$(FIRMWARE)'
	@for i in 1 2 3 4 5 6 7 8 9 10; \
	 do \
	   [ -e '${VIDEO_DEVICE}' ] && break; \
	   printf '.'; \
	   sleep 1; \
	done; \
	sleep 1; \
	printf '.\n'

.PHONY: cheese
cheese: all
	-[ ! -e '${VIDEO_DEVICE}' ] && $(MAKE) download
	[ -e '${VIDEO_DEVICE}' ] && cheese -d '${VIDEO_DEVICE}'


.PHONY: clean
clean:
	${RM} frame*.png
	${RM} animated*.gif
