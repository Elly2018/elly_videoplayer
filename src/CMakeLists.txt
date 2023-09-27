# SPDX-License-Identifier: Unlicense

file(GLOB_RECURSE SRC *.h *.hpp *.cpp)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>DLL")

target_sources( ${PROJECT_NAME}
    PRIVATE
	${SRC}
)

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "src"
)
