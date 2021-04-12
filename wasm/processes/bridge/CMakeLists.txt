cmake_minimum_required( VERSION 3.5 )
project(process)
add_definitions(
    -DNDEBUG
)
add_executable(bridge
    process.cpp
)
target_link_libraries(bridge PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(bridge PRIVATE boost)
set_target_properties(bridge PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
