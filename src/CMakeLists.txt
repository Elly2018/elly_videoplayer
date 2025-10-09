# SPDX-License-Identifier: Unlicense

file(GLOB_RECURSE SRC src/*.h src/*.hpp src/*.cpp)

target_sources( ${PROJECT_NAME}
    PRIVATE
	${SRC}
)

get_target_property(target_options ${PROJECT_NAME} COMPILE_OPTIONS)
list(REMOVE_ITEM target_options "-Wpedantic")
list(REMOVE_ITEM target_options "-Wno-unused-parameter")
list(REMOVE_ITEM target_options "-Wunused")
list(REMOVE_ITEM target_options "-Wcast-align")
list(REMOVE_ITEM target_options "-Wall")
list(REMOVE_ITEM target_options "-Wextra")
list(REMOVE_ITEM target_options "-Woverloaded-virtual")
list(REMOVE_ITEM target_options "-Wshadow")
set_property(TARGET ${PROJECT_NAME} PROPERTY COMPILE_OPTIONS ${target_options})
message(${target_options})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "gd_videoplayer")
set_property(TARGET ${PROJECT_NAME} PROPERTY C_EXTENSIONS ON)
file (STRINGS "platform.txt" Platform)
message("Target platform: ${Platform}")
if( "${Platform}" STREQUAL "Windows" )
# Windows config
message("Detect Windows")
target_link_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-win64/lib" )
target_link_libraries(${PROJECT_NAME} PRIVATE avcodec.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avdevice.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avfilter.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avformat.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avutil.lib)
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
target_link_libraries(${PROJECT_NAME} PRIVATE libswresample.so)
target_link_libraries(${PROJECT_NAME} PRIVATE libswscale.so)
target_include_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-linux64/include" )
target_include_directories( ${PROJECT_NAME} PUBLIC "lib/ffmpeg-linux64/gen/include" )
endif()

target_include_directories( ${PROJECT_NAME} PRIVATE "src" )
