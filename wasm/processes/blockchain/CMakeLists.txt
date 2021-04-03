cmake_minimum_required( VERSION 3.5 )
project(process)
add_definitions(
    -DNDEBUG
)
add_executable(blockchain
    process.cpp
)
target_link_libraries(blockchain PRIVATE nlohmann_json::nlohmann_json)
set_target_properties(blockchain PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})