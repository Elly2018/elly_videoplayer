#!/bin/sh
cd ..
cd src
echo Linux>platform.txt
cd ..
cmake -B elly_player_gdextension_linux -G"Visual Studio 17 2022" -DPlatfrom=Linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=elly_player_gdextension_linux-install elly_videoplayer