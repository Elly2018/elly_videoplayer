@echo off
set path=%path%;%cd%
cd ../lib
curl --output ffmpeg.zip -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip
7z x ffmpeg.zip -offmpeg-master-latest-win64-gpl-shared
ren ffmpeg-master-latest-win64-gpl-shared ffmpeg-win64
del ffmpeg.zip
pause