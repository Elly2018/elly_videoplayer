@echo off
cd ../..
copy "src\ffmpeg\bin\avcodec-60.dll" %projectpath% /Y
copy "src\ffmpeg\bin\avdevice-60.dll" %projectpath% /Y
copy "src\ffmpeg\bin\avfilter-9.dll" %projectpath% /Y
copy "src\ffmpeg\bin\avformat-60.dll" %projectpath% /Y
copy "src\ffmpeg\bin\avutil-58.dll" %projectpath% /Y
copy "src\ffmpeg\bin\postproc-57.dll" %projectpath% /Y
copy "src\ffmpeg\bin\swresample-4.dll" %projectpath% /Y
copy "src\ffmpeg\bin\swscale-7.dll" %projectpath% /Y
pause
