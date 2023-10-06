# SPDX-License-Identifier: Unlicense

file(GLOB_RECURSE SRC src/*.h src/*.hpp src/*.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE "/WX-")

target_sources( ${PROJECT_NAME}
    PRIVATE
	${SRC}
)

target_link_directories( ${PROJECT_NAME} PUBLIC
  "ffmpeg/lib"
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "gd_videoplayer")

if( WIN32 )
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
target_link_libraries(${PROJECT_NAME} PRIVATE avcodec.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE avdevice.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE avfilter.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE avformat.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE avutil.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE postproc.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE swresample.dll.a)
target_link_libraries(${PROJECT_NAME} PRIVATE swscale.dll.a)
endif()

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "src"
)

target_include_directories( ${PROJECT_NAME}
	PUBLIC
		"ffmpeg/include"
)
