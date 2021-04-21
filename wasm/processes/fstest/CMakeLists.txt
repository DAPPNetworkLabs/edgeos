cmake_minimum_required( VERSION 3.5 )
project(process)
add_definitions(
    -DNDEBUG
)
add_executable(fstest
    process.cpp
)
target_link_libraries(fstest PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(fstest PRIVATE boost)
set_target_properties(fstest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
