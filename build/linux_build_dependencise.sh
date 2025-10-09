#!/bin/sh
cd ..
cp --verbose -f "src/lib/ffmpeg-linux64/lib/libavcodec.so" $1
cp --verbose -f "src/lib/ffmpeg-linux64/lib/libavdevice.so" $1
cp --verbose -f "src/lib/ffmpeg-linux64/lib/libavfilter.so" $1
cp --verbose -f "src/lib/ffmpeg-linux64/lib/libavformat.so" $1
cp --verbose -f "src/lib/ffmpeg-linux64/lib/libavutil.so" $1
cp --verbose -f "src/lib/ffmpeg-linux64/lib/libswresample.so" $1
cp --verbose -f "src/lib/ffmpeg-linux64/lib/libswscale.so" $1
