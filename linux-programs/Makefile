# Copyright Bitmark Inc 2015-2015

VIDEO_DEVICE ?= /dev/video1
FIRMWARE ?= ../bitmark-microscope/bitmark-microscope.img
DOWNLOAD_FX3 = ../../../../cyusb_linux_1.0.4/src/download_fx3

FRAMES ?= 300
BRIGHTNESS ?= 0
SHARPNESS ?= 0x00a0
CONTRAST ?= 15
LEDS ?= 0x0f
FRAMES_OUT ?= frames.data

ANIMATION_DELAY ?= 15
ANIMATION_SIZE ?= 300x300
#ANIMATION_OFFSET ?= +0+0
ANIMATION_OFFSET ?= +910+390
ANIMATION_EXTRACT = ${ANIMATION_SIZE}${ANIMATION_OFFSET}
ANIMATION_REPAGE = ${ANIMATION_SIZE}$(subst +,-,${ANIMATION_OFFSET})


CAPTURE_OPTS = -c '${FRAMES}' -t
CAPTURE_OPTS += -o '${FRAMES_OUT}'
CAPTURE_OPTS += -d '${VIDEO_DEVICE}'
CAPTURE_OPTS += --brightness='${BRIGHTNESS}'
CAPTURE_OPTS += --sharpness='${SHARPNESS}'
CAPTURE_OPTS += --contrast='${CONTRAST}'
CAPTURE_OPTS += --leds='${LEDS}'

CREATE_PNG_OPTS = --prefix='frame'
CREATE_PNG_OPTS += --verbose
CREATE_PNG_OPTS += --slider
CREATE_PNG_OPTS += --number
CREATE_PNG_OPTS += --embed='8192'


CFLAGS += -std=gnu99 -Wall -Werror
CFLAGS += $(shell pkg-config --cflags libpng12)
#CFLAGS += -g
CFLAGS += -O3

LFLAGS += -lbsd
LFLAGS += $(shell pkg-config --libs libpng12)
#LFLAGS += -g
LFLAGS += -O3

RM = rm -f


.PHONY: all
all: capture create-png list-controls test-leds

CAPTURE_OBJECTS = capture.o
capture: ${CAPTURE_OBJECTS}
	${CC} ${CFLAGS} -o '$@' ${CAPTURE_OBJECTS} ${LFLAGS}

CREATE_PNG_OBJECTS = create-png.o
CREATE_PNG_OBJECTS += ahd_bayer.o
create-png:  ${CREATE_PNG_OBJECTS}
	${CC} ${CFLAGS}  -o '$@' ${CREATE_PNG_OBJECTS} ${LFLAGS}

LIST_CONTROLS_OBJECTS = list-controls.o
list-controls: ${LIST_CONTROLS_OBJECTS}
	${CC} ${CFLAGS} -o '$@' ${LIST_CONTROLS_OBJECTS} ${LFLAGS}

TEST_LEDS_OBJECTS = test-leds.o
test-leds: ${TEST_LEDS_OBJECTS}
	${CC} ${CFLAGS} -o '$@'  ${TEST_LEDS_OBJECTS} ${LFLAGS}

create-png.o: ahd_bayer.h
ahd_bayer.o: ahd_bayer.h

%.o: %.c
	${CC} -c ${CFLAGS} -o '$@' '$<'


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
	${RM} '${FRAMES_OUT}'
	-[ ! -e '${VIDEO_DEVICE}' ] && $(MAKE) download
	[ -e '${VIDEO_DEVICE}' ] && ./capture ${CAPTURE_OPTS}

.PHONY: dec
dec: create-png
	./create-png ${CREATE_PNG_OPTS} '${FRAMES_OUT}'



#BOX = 909,539,1011,541
BOX = 767,539,1153,541
.PHONY: full
full:
	${RM} animated_full.gif
	convert -dispose previous -delay '${ANIMATION_DELAY}' -loop 0 \
	  -fill none -stroke black \
	  -draw 'rectangle ${BOX}' \
	  'frame*.png' animated_full.gif

.PHONY: small
small:
	${RM} animated_small.gif
	convert -dispose previous -delay '${ANIMATION_DELAY}' -loop 0 \
	  -repage '${ANIMATION_REPAGE}' \
	  -extract '${ANIMATION_EXTRACT}' \
	  -fill none -stroke black \
	  -draw 'rectangle 99,149,201,151' \
	  'frame*.png' animated_small.gif


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
	${RM} *.o
	${RM} frame*.png
	${RM} animated*.gif
