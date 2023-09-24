@echo off

copy FFmpeg\libavcodec\avcodec.dll project\addons\example\bin\win /Y
copy FFmpeg\libavcodec\avcodec-60.dll project\addons\example\bin\win /Y

copy FFmpeg\libavutil\avutil.dll project\addons\example\bin\win /Y
copy FFmpeg\libavutil\avutil-58.dll project\addons\example\bin\win /Y

copy FFmpeg\libavformat\avformat.dll project\addons\example\bin\win /Y
copy FFmpeg\libavformat\avformat-60.dll project\addons\example\bin\win /Y

scons platform=windows use_mingw=1
pause