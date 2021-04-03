cmake_minimum_required( VERSION 3.5 )

project(edgeos)

set(IS_WASM YES)
set(ROOT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/..)
set(ROOT_BINARY_DIR ${CMAKE_BINARY_DIR}/..)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(../libs libs)
add_subdirectory(../plugins plugins)
add_subdirectory(../processes processes)
add_subdirectory(../solidity solidity)
