#!/bin/sh
cd ../lib
curl --output ffmpeg.tar.xz -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-gpl-shared.tar.xz
tar -xf ffmpeg.tar.xz
mv ffmpeg-master-latest-linux64-gpl-shared ffmpeg-linux64
rm ./ffmpeg.tar.xz
