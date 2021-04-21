cmake_minimum_required( VERSION 3.5 )
project(memfs)
add_definitions(
    -DNDEBUG
)
add_executable(memfs
    memfs.c
)
set_target_properties(memfs PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
