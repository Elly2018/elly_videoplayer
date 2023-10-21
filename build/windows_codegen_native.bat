@echo off
cd ..
cd src
echo Windows>platform.txt
cd ..
cmake -B NativeExtensionTemplate-build -G"Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=NativeExtensionTemplate-install elly_videoplayer
pause