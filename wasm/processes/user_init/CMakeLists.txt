cmake_minimum_required( VERSION 3.5 )
project(user_init)
add_definitions(
    -DNDEBUG
)
add_executable(user_init
    process.cpp
)
target_link_libraries(user_init PRIVATE nlohmann_json::nlohmann_json)

set_target_properties(user_init PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
