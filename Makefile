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
IAPETUS_SRC ?= $(shell pwd)/iapetus

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
OBJCOPY = $(CROSS_COMPILE)objcopy

BASE_CFLAGS = -O2 -m2 -Wall -ggdb3 -ffunction-sections -fdata-sections -fno-stack-protector

CFLAGS ?= $(BASE_CFLAGS)
CFLAGS += -I$(IAPETUS_SRC)/src
CFLAGS += -I. -Idisc_format -Igui -Ilibsatiator

IAPETUS_LIBDIR=iapetus-build/src

LDFLAGS += -nostdlib -L$(IAPETUS_LIBDIR) -Wl,--gc-sections -Lout/

VERSION ?= $(shell git describe --always --dirty --match aotsrintsoierats) $(shell date +%y%m%d%H%M%S)
CFLAGS += -DVERSION='"$(VERSION)"'

SRCS := init.c gui/fade.c libsatiator/satiator.c syscall.c jhloader.c main.c gui/gmenu.c gui/filelist.c clock.c disc_format/cdparse.c disc_format/cue2desc.c ar.c diagnostics.c elfloader.c

ifeq ($(DEBUG), yes)
	CFLAGS += -DDEBUG
endif

OBJS := $(addprefix out/,$(SRCS:.c=.o))

default: $(IAPETUS_LIBDIR)/libiapetus.a out/menu.bin

out/menu.bin: ip.bin out/menu_code.bin
	cat $^ > $@

out/%_code.bin: out/%.elf
	$(OBJCOPY) -O binary -R .noload $< $@

out/menu.elf: menu.ld $(OBJS) $(IAPETUS_LIBDIR)/libiapetus.a out/libc-nosys.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -T menu.ld -Wl,-Map=out/menu.map $(OBJS) -liapetus -lc-nosys -lgcc

out/%.o: %.c $(CC_DEPS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

out/%.o: %.s $(CC_DEPS)
	@mkdir -p $(dir $@)
	$(AS) $< -o $@

NEWLIB_LIBC=$(shell CROSS_COMPILE=$(CROSS_COMPILE) ./find-libc)
out/libc-nosys.a: $(NEWLIB_LIBC)
	# strip out the syscalls; we will supply them
	echo in $<
	echo out $@
	cp $< $@
	$(CROSS_COMPILE)ar d $@ lib_a-syscalls.o

clean:
	rm -rf out/*

distclean:
	rm -rf out iapetus-build

iapetus-build/Makefile: $(CC_DEPS)
	mkdir -p iapetus-build
	(cd iapetus-build; CC=$(CC) CXX=$(CC) cmake $(IAPETUS_SRC) -DCMAKE_SHARED_LINKER_FLAGS:STRING="$(LDFLAGS)" -DCMAKE_C_FLAGS='-DREENTRANT_SYSCALLS_PROVIDED $(BASE_CFLAGS)')

iapetus-build/src/libiapetus.a: iapetus-build/Makefile
	+(cd iapetus-build; make -f src/CMakeFiles/iapetus.dir/build.make VERBOSE=1 src/libiapetus.a)

.DELETE_ON_ERROR:
