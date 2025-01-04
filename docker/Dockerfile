FROM debian:bullseye AS crosstool

RUN apt update
RUN apt install -y gcc g++ gperf bison flex texinfo help2man make libncurses5-dev \
    python3-dev autoconf automake libtool libtool-bin gawk wget bzip2 xz-utils unzip \
    patch libstdc++6 rsync git

ENV CROSSTOOL_VERSION 1.24.0

RUN wget http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-${CROSSTOOL_VERSION}.tar.xz
RUN tar xf crosstool-ng-${CROSSTOOL_VERSION}.tar.xz
WORKDIR crosstool-ng-${CROSSTOOL_VERSION}
RUN ./configure
RUN make
RUN make install

RUN useradd -m -u 1000 -s /bin/bash build
USER 1000
ENV HOME /home/build

RUN mkdir ${HOME}/crossbuild-sh
WORKDIR ${HOME}/crossbuild-sh
ADD sh-config .config
RUN ct-ng build

FROM debian:bullseye

COPY --from=crosstool /home/build/x-tools/sh-none-elf /usr/local/
ENV PATH ${PATH}:/usr/local/sh-none-elf/bin

RUN apt update
RUN apt install -y cmake git
