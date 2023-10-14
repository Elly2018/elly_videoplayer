# SPDX-License-Identifier: Unlicense

file(GLOB_RECURSE SRC src/*.h src/*.hpp src/*.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE "/WX-")
target_compile_options(${PROJECT_NAME} PRIVATE "/O2")

set_property(TARGET ${PROJECT_NAME} PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DLL>")

target_sources( ${PROJECT_NAME}
    PRIVATE
	${SRC}
)

target_link_directories( ${PROJECT_NAME} PUBLIC
  "ffmpeg/lib"
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "gd_videoplayer")
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

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "src"
)

target_include_directories( ${PROJECT_NAME}
	PUBLIC
		"ffmpeg/include"
)
