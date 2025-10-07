@echo off
cd ../lib
curl --output ffmpeg.zip -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip
tar -xf ffmpeg.zip
rename ffmpeg-master-latest-win64-gpl-shared ffmpeg-win64
del ffmpeg.zip
pause