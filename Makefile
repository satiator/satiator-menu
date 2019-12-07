# Copyright (c) 2015 James Laird-Wah
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.


# whether to enable debug printf()s, etc.
# set to yes to enable
DEBUG := yes

# Set to point to your toolchain
CROSS_COMPILE ?= sh-none-elf-

# Absolute location of Iapetus source tree
IAPETUS_SRC ?= $(shell pwd)/../iapetus
NEWLIB_SRC ?= $(shell pwd)/../newlib-2.2.0.20150423

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS ?= -O2 -m2 -nostdlib -Wall -ggdb3 -ffunction-sections -fdata-sections
CFLAGS += -I$(IAPETUS_SRC)/src
CFLAGS += -I. -Idisc_format -Igui

IAPETUS_LIBDIR=iapetus-build/src

NEWLIB_ARCH=$(patsubst %-,%,$(basename $(CROSS_COMPILE)))
NEWLIB_LIBDIR=newlib-build/prefix/$(NEWLIB_ARCH)/lib

LDFLAGS += -L$(IAPETUS_LIBDIR) -L$(NEWLIB_LIBDIR) -Wl,--gc-sections

VERSION ?= $(shell git describe --always --dirty --match aotsrintsoierats) $(shell date +%y%m%d%H%M%S)
CFLAGS += -DVERSION='"$(VERSION)"'

SRCS := init.c gui/fade.c satiator.c syscall.c jhloader.c main.c gui/gmenu.c disc_format/cdparse.c disc_format/cue2desc.c

ifeq ($(DEBUG), yes)
	CFLAGS += -DDEBUG
endif

OBJS := $(addprefix out/,$(notdir $(SRCS:.c=.o)))

default: out/menu.bin

out/menu.bin: ip.bin out/menu_code.bin
	cat $^ > $@

out/%_code.bin: out/%.elf
	$(OBJCOPY) -O binary $< $@

out/menu.elf: menu.ld $(OBJS) $(IAPETUS_LIBDIR)/libiapetus.a $(NEWLIB_LIBDIR)/libc-nosys.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -T menu.ld -Wl,-Map=out/menu.map $(OBJS) -liapetus -lc-nosys -lgcc

out/%.o: %.c out/.dir_exists
	$(CC) $(CFLAGS) -c $< -o $@

out/%.o: %.s out/.dir_exists
	$(AS) $< -o $@

out/%.o: disc_format/%.c
	$(CC) $(CFLAGS) -c $< -o $@

out/%.o: gui/%.c
	$(CC) $(CFLAGS) -c $< -o $@

out/.dir_exists:
	mkdir -p out
	touch out/.dir_exists

clean:
	rm -f out/*

distclean:
	rm -rf out iapetus-build newlib-build

iapetus-build/Makefile:
	mkdir -p iapetus-build
	(cd iapetus-build; CC=$(CC) CMAKE_C_FLAGS=-ggdb3 cmake $(IAPETUS_SRC))

iapetus-build/src/libiapetus.a: iapetus-build/Makefile
	(cd iapetus-build; make)

newlib-build/Makefile:
	mkdir -p newlib-build
	(cd newlib-build; NEWLIB_SRC=$(NEWLIB_SRC) ../build-newlib.sh)

newlib-build/prefix/$(NEWLIB_ARCH)/lib/libc.a: newlib-build/Makefile
	(cd newlib-build; make && make install)

# newlib disregards the --disable-newlib-supplied-syscalls configure argument on most arches >:(
newlib-build/prefix/$(NEWLIB_ARCH)/lib/libc-nosys.a: newlib-build/prefix/$(NEWLIB_ARCH)/lib/libc.a
	cp $< $@
	$(CROSS_COMPILE)ar d $@ lib_a-syscalls.o


.DELETE_ON_ERROR:
