cmake_minimum_required(VERSION 3.4.0)

set(CMAKE_EXECUTABLE_SUFFIX_C .wasm)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .wasm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
set(WASM_EXPORTS "")
set(WASM_EXPORTS "${WASM_EXPORTS},--export-dynamic")
set(WASM_EXPORTS "${WASM_EXPORTS},--strip-all")
set(WASM_EXPORTS "${WASM_EXPORTS},--export-table")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,${WASM_EXPORTS}")
