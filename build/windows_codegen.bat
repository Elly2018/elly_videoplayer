@echo off
cd ..
cd src
echo Windows>platform.txt
cd ..
cmake -B elly_player_gdextension_windows -G"Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=elly_player_gdextension_windows-install elly_videoplayer
pause