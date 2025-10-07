# SPDX-License-Identifier: Unlicense

file(GLOB_RECURSE SRC src/*.h src/*.hpp src/*.cpp)

target_sources( ${PROJECT_NAME}
    PRIVATE
	${SRC}
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "gd_videoplayer")
file (STRINGS "platform.txt" Platform)
message("Target platform: ${Platform}")
if( "${Platform}" STREQUAL "Windows" )
# Windows config
message("Detect Windows")
target_compile_options(${PROJECT_NAME} PRIVATE "/WX-")
target_link_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-win64/lib" )
target_link_libraries(${PROJECT_NAME} PRIVATE avcodec.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avdevice.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avfilter.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avformat.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avutil.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE postproc.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE swresample.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE swscale.lib)
target_include_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-win64/include" )
target_include_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-win64/gen/include" )
else()
# MacOS or Linux config
message("Detect Linux")
target_link_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-linux64/lib" )
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
target_link_libraries(${PROJECT_NAME} PRIVATE libavcodec.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libavdevice.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libavfilter.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libavformat.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libavutil.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libpostproc.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libswresample.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libswscale.so)
target_include_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-linux64/include" )
target_include_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-linux64/gen/include" )
endif()

target_include_directories( ${PROJECT_NAME} PRIVATE "src" )
