# Copyright Bitmark Inc 2015-2015

VIDEO_DEVICE ?= /dev/video1
FIRMWARE ?= ../bitmark-microscope/bitmark-microscope.img

.PHONY: all
all: capture create-png

capture: capture.c
	gcc -std=c99 -Wall -Werror -o capture capture.c -lbsd

create-png: create-png.c
	gcc -std=c99 -Wall -Werror  -o create-png create-png.c $$(pkg-config --cflags --libs libpng12)

.PHONY: cap
cap: capture
	-[ ! -e "${VIDEO_DEVICE}" ] && $(MAKE) download
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	[ -e "${VIDEO_DEVICE}" ] && ./capture -c 100 -t -o frames.data -d "${VIDEO_DEVICE}"

.PHONY: dec
dec: create-png
	./create-png

.PHONY: download
download:
	$(DOWNLOAD_FX3) -t RAM -i "$(FIRMWARE)"

.PHONY: cheese
cheese: all
	-[ ! -e "${VIDEO_DEVICE}" ] && $(MAKE) download
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	[ -e "${VIDEO_DEVICE}" ] && cheese -d "${VIDEO_DEVICE}"
