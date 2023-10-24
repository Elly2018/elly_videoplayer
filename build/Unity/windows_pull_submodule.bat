@echo off
cd ../..
git clone https://chromium.googlesource.com/libyuv/libyuv
cd src
curl --output unitynative.zip --ssl-no-revoke -L -O https://github.com/Unity-Technologies/NativeRenderingPlugin/archive/refs/heads/master.zip
tar -xf unitynative.zip
rename NativeRenderingPlugin-master unity-native
del unitynative.zip
pause