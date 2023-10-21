@echo off
cd ../..
cd src
echo Windows>platform.txt
cd ..
cmake -B GDExtensionTemplate-build -G"Visual Studio 17 2022" -DEngine=Godot -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=GDExtensionTemplate-install elly_videoplayer
pause