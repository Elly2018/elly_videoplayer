@echo off
cd ..
cmake -B GDExtensionTemplate-build -G"Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=GDExtensionTemplate-install gd_videoplayer
pause