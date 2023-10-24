@echo off
cd ../..
git clone -b 4.1 https://github.com/godotengine/godot-cpp.git
git clone https://chromium.googlesource.com/libyuv/libyuv
cd src
curl --output ffmpeg.zip --ssl-no-revoke -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip
tar -xf ffmpeg.zip
rename ffmpeg-master-latest-win64-gpl-shared ffmpeg
del ffmpeg.zip
pause