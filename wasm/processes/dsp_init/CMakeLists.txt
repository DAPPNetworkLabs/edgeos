cmake_minimum_required( VERSION 3.5 )
project(dsp_init)
add_definitions(
    -DNDEBUG
)
add_executable(dsp_init
    process.cpp
)
target_link_libraries(dsp_init PRIVATE nlohmann_json::nlohmann_json)

set_target_properties(dsp_init PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
