# Copyright (c) 2015 James Laird-Wah
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.


# Set to point to your toolchain
CROSS_COMPILE ?= sh-none-elf-

# Absolute location of Iapetus source tree
IAPETUS_SRC ?= $(shell pwd)/../iapetus

CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS ?= -O2 -m2 -nostdlib -Wall
CFLAGS += -I$(IAPETUS_SRC)/src

LDFLAGS += -Liapetus-build/src

SRCS := init.c fade.c satisfier.c libc.c
OBJS := $(addprefix out/,$(SRCS:.c=.o))

out/menu.bin: ip.bin out/menu_code.bin
	cat $^ > $@

out/menu_code.bin: out/menu.elf
	$(OBJCOPY) -O binary $< $@

out/menu.elf: ldscript.ld $(OBJS) iapetus-build/src/libiapetus.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -T ldscript.ld -Wl,-Map out/menu.map $(OBJS) -liapetus -lgcc

out/%.o: %.c out/.dir_exists
	$(CC) $(CFLAGS) -c $< -o $@

out/.dir_exists:
	mkdir -p out
	touch out/.dir_exists

clean:
	rm out/*

iapetus-build:
	mkdir iapetus-build
	(cd iapetus-build; CC=$(CC) cmake $(IAPETUS_SRC))

iapetus-build/src/libiapetus.a: iapetus-build
	(cd iapetus-build; make)

.DELETE_ON_ERROR:
