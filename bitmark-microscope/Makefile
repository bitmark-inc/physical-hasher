# Copyright Bitmark Inc 2015-2015

FX3FWROOT = ../../..

VIDEO_DEVICE ?= /dev/video1

ELF2IMG = $(FX3FWROOT)/util/elf2img/elf2img
DOWNLOAD_FX3 = ../../../../cyusb_linux_1.0.4/src/download_fx3

MODULE = bitmark-microscope

.PHONY: all
all: $(MODULE).img


.PHONY: download all
download:
	$(DOWNLOAD_FX3) -t RAM -i "$(MODULE).img"

.PHONY: cheese
cheese: all
	-[ ! -e "${VIDEO_DEVICE}" ] && $(MAKE) download
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	-[ ! -e "${VIDEO_DEVICE}" ] && sleep 1
	[ -e "${VIDEO_DEVICE}" ] && cheese -d "${VIDEO_DEVICE}"

include $(FX3FWROOT)/firmware/common/cx3_build_config.mak

USER_CFLAGS += -Werror -std=c99


#SOURCE += $(MODULE).c
SOURCE += ar0330.c
SOURCE += cycx3_uvc.c
SOURCE += cycx3_uvcdescr.c
SOURCE += sensor_i2c.c
SOURCE += cyfxtx.c
SOURCE_ASM += cyfx_gcc_startup.S

C_OBJECT = $(SOURCE:%.c=./%.o)
A_OBJECT = $(SOURCE_ASM:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

$(MODULE).img: compile

%.img: %.elf
	$(ELF2IMG) -vectorload yes -i "$<" -o "$@"

$(MODULE).$(EXEEXT): $(A_OBJECT) $(C_OBJECT)
	$(LINK)

$(C_OBJECT) : %.o : %.c ar0330.h cycx3_uvc.h cycx3_uvcdescr.h
	$(COMPILE)

$(A_OBJECT) : %.o : %.S
	$(ASSEMBLE)

clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./*.o

compile: $(C_OBJECT) $(A_OBJECT) $(EXES)
