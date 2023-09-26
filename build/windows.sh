#!/bin/sh

cd ../ffmpeg

ARCH=$(uname -m)

./configure \
	--enable-cross-compile \
	--enable-shared \
	--disable-zlib \
	--disable-programs \
	--disable-doc \
	--disable-manpages \
	--disable-podpages \
	--disable-txtpages \
	--disable-ffplay \
	--disable-ffprobe \
	--disable-ffmpeg \
	--disable-x86asm \
