#!/bin/sh
cd ../lib
curl --output ffmpeg.tar.xz -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2025-10-06-13-37/ffmpeg-N-121334-g6231fa7fb7-linux64-gpl-shared.tar.xz
tar -xf ffmpeg.tar.xz
mv ffmpeg-N-121334-g6231fa7fb7-linux64-gpl-shared ffmpeg-linux64
rm ./ffmpeg.tar.xz
