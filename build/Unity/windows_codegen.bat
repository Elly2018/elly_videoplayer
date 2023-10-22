@echo off
cd ../..
cd src
echo Windows>platform.txt
cd ..
cmake -B UnityExtensionTemplate-build -G"Visual Studio 17 2022" -DEngine=Unity -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=UnityExtensionTemplate-install elly_videoplayer
pause