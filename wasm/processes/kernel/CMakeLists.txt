cmake_minimum_required( VERSION 3.5 )
project(kernel)
add_definitions(
    -DNDEBUG
)
add_executable(kernel
    kernel.cpp
)

target_link_libraries(kernel PRIVATE nlohmann_json::nlohmann_json)
set_target_properties(kernel PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})