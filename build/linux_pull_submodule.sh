#!/bin/sh
cd ..
git clone -b 4.1 https://github.com/godotengine/godot-cpp.git
cd src
curl --output ffmpeg.zip -L -O https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip
tar -xf ffmpeg.zip
rename ffmpeg-master-latest-win64-gpl-shared ffmpeg
rm ./ffmpeg.zip