# SPDX-License-Identifier: Unlicense

file(GLOB_RECURSE SRC *.h *.hpp *.cpp)

target_sources( ${PROJECT_NAME}
    PRIVATE
	${SRC}
)

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "src"
)
