#!/bin/bash

pwd

set -e

PREFIX=`pwd`/prefix/
PROGPREFIX=sh2-elf-

mkdir -p ${PREFIX}

function disable_flags () {
    for flag in io-pos-args io-c99-formats register-fini io-long-long io-long-double mb atexit-dynamic-alloc global-atexit wide-orient multilib malloc-debugging multithread iconv io-float supplied-syscalls; do
        echo -n "--disable-newlib-${flag} "
    done
}


export TARGET=sh-elf
export CC=${PROGPREFIX}gcc
export LD=${PROGPREFIX}ld
export AS=${PROGPREFIX}as
export AR=${PROGPREFIX}ar
export RANLIB=${PROGPREFIX}ranlib
export BUILD_CC=sh-elf-gcc
export CC_FOR_TARGET=${PROGPREFIX}gcc
export LD_FOR_TARGET=${PROGPREFIX}ld
export AS_FOR_TARGET=${PROGPREFIX}as
export AR_FOR_TARGET=${PROGPREFIX}ar
export RANLIB_FOR_TARGET=${PROGPREFIX}ranlib
export TARGET_CFLAGS="-m2 -fno-stack-protector"
${NEWLIB_SRC}/configure --prefix=${PREFIX} \
                        --program-prefix=${PROGPREFIX} \
                        --target=${TARGET} \
                        --enable-lite-exit \
                        --enable-newlib-nano-formatted-io \
                        --enable-newlib-nano-malloc \
                        --enable-target-optspace \
                        --enable-newlib-reent-small \
                        --disable-multilib $(disable_flags)

make -j8
make install
