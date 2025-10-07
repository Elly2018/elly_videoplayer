#!/bin/sh
cd ../lib
curl --output ffmpeg.zip -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-gpl-shared.tar.xz
unzip ffmpeg.zip
mv ffmpeg-master-latest-win64-gpl-shared ffmpeg-linux64
rm ./ffmpeg.zip
