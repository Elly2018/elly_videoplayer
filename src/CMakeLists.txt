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

target_link_libraries(${PROJECT_NAME} PRIVATE avcodec.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avdevice.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avfilter.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avformat.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avutil.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE postproc.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE swresample.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE swscale.lib)

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "src"
)

target_include_directories( ${PROJECT_NAME}
	PUBLIC
		"ffmpeg/include"
)
