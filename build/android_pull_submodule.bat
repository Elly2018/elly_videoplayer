@echo off
cd ..
git clone -b 4.1 https://github.com/godotengine/godot-cpp.git
cd src
curl --output ffmpeg-kit.aar -L -O https://github.com/arthenica/ffmpeg-kit/releases/download/v6.0.LTS/ffmpeg-kit-audio-6.0-2.LTS.aar
pause