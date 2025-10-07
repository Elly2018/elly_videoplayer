@echo off
cd ..
copy "lib\ffmpeg-win64\bin\avcodec-62.dll" %projectpath% /Y
copy "lib\ffmpeg-win64\bin\avdevice-62.dll" %projectpath% /Y
copy "lib\ffmpeg-win64\bin\avfilter-11.dll" %projectpath% /Y
copy "lib\ffmpeg-win64\bin\avformat-62.dll" %projectpath% /Y
copy "lib\ffmpeg-win64\bin\avutil-60.dll" %projectpath% /Y
copy "lib\ffmpeg-win64\bin\swresample-6.dll" %projectpath% /Y
copy "lib\ffmpeg-win64\bin\swscale-9.dll" %projectpath% /Y
pause
