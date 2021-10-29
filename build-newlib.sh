#!/bin/bash

pwd

set -e

PREFIX=`pwd`/prefix/

mkdir -p ${PREFIX}

function disable_flags () {
    for flag in io-pos-args io-c99-formats register-fini io-long-long io-long-double mb atexit-dynamic-alloc global-atexit wide-orient multilib malloc-debugging multithread iconv io-float supplied-syscalls; do
        echo -n "--disable-newlib-${flag} "
    done
}

export TARGET_CFLAGS="-m2 -fno-stack-protector"
${NEWLIB_SRC}/configure --prefix=${PREFIX} \
                        --target=sh-none-elf $(disable_flags) \
                        --enable-lite-exit \
                        --enable-newlib-nano-formatted-io \
                        --enable-newlib-nano-malloc \
                        --enable-target-optspace \
                        --enable-newlib-reent-small \
                        --disable-multilib

make -j8
make install
