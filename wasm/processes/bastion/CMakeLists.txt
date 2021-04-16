cmake_minimum_required( VERSION 3.5 )
project(process)
add_definitions(
    -DNDEBUG
)
add_executable(bastion
    process.cpp
)
target_link_libraries(bastion PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(bastion PRIVATE boost)
set_target_properties(bastion PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
