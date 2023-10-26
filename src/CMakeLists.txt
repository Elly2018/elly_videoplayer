# SPDX-License-Identifier: Unlicense
file(GLOB_RECURSE CORESRC 
	src/encoder/*.hpp src/encoder/*.h src/encoder/*.cpp src/encoder/*.c
	src/decoder/*.hpp src/decoder/*.h src/decoder/*.cpp src/decoder/*.c
	src/interface/*.hpp src/interface/*.h src/interface/*.cpp src/interface/*.c
	src/DecodeConfig.h)

file(GLOB_RECURSE GDSRC src/gd/*.hpp src/gd/*.h src/gd/*.cpp src/gd/*.c)
file(GLOB_RECURSE UnitySRC src/unity/*.hpp src/unity/*.h src/unity/*.cpp src/unity/*.c)
file(GLOB_RECURSE UnityNativeSRC unity-native/PluginSource/source/*.hpp unity-native/PluginSource/source/*.h unity-native/PluginSource/source/*.cpp unity-native/PluginSource/source/*.c)
file(GLOB_RECURSE UnrealSRC src/unreal/*.hpp src/unreal/*.h src/unreal/*.cpp src/unreal/*.c)
file(GLOB_RECURSE NativeSRC src/Logger.h)

target_include_directories( ${PROJECT_NAME} PRIVATE "src")

if( "${Engine}" STREQUAL "Godot" )
	list(APPEND SRC "${CORESRC}" "${GDSRC}")
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "gd_videoplayer")
elseif( "${Engine}" STREQUAL "Unity" )
	list(APPEND SRC "${CORESRC}" "${UnitySRC}" "${UnityNativeSRC}")
	target_include_directories( ${PROJECT_NAME} PRIVATE "unity-native/PluginSource/source")
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "unity_videoplayer")
	target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
elseif( "${Engine}" STREQUAL "Unreal" )
	list(APPEND SRC "${CORESRC}" "${UnrealSRC}")
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "unreal_videoplayer")
else()
	list(APPEND SRC "${CORESRC}" "${NativeSRC}")
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "native_videoplayer")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE "/WX-")

set_property(TARGET ${PROJECT_NAME} PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DLL>")

target_sources( ${PROJECT_NAME}
    PRIVATE
	${SRC}
)

target_link_directories( ${PROJECT_NAME} PUBLIC
  "ffmpeg/lib"
)

file (STRINGS "Platform.txt" Platform)
message("Target platform: ${Platform}")
if( "${Platform}" STREQUAL "Windows" )
# Windows config
target_link_libraries(${PROJECT_NAME} PRIVATE avcodec.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avdevice.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avfilter.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avformat.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avutil.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE postproc.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE swresample.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE swscale.lib)
else()
# MacOS or Linux config
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
target_link_libraries(${PROJECT_NAME} PRIVATE libavcodec.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE libavdevice.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE libavfilter.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE libavformat.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE libavutil.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE libpostproc.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE libswresample.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE libswscale.dll.a)
endif()

target_include_directories( ${PROJECT_NAME} PUBLIC "ffmpeg/include")
