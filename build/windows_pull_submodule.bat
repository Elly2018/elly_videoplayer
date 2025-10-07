@echo off
set path=%path%;%cd%
cd ../src/lib
curl --output ffmpeg.zip -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2025-10-06-13-37/ffmpeg-n8.0-16-gd8605a6b55-win64-gpl-shared-8.0.zip
7z x ffmpeg.zip
ren ffmpeg-n8.0-16-gd8605a6b55-win64-gpl-shared-8.0 ffmpeg-win64
del ffmpeg.zip
pause