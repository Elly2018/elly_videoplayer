#!/bin/sh
cd ..
cd src
echo Linux>platform.txt
cd ..
cmake -B GDExtensionTemplate-build -G"Visual Studio 17 2022" -DPlatfrom=Linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=GDExtensionTemplate-install gd_videoplayer