cmake_minimum_required( VERSION 3.5 )
project(evm)

set(WASI TRUE)
set(IS_WASM YES)
set(WASI_SDK_PREFIX "/home/talm/.local/lib/python3.8/site-packages/wasienv-storage/sdks/12/wasi-sdk-12.0")

set (CMAKE_SYSROOT ${WASI_SDK_PREFIX}/share/wasi-sysroot)

set(CMAKE_SYSTEM_NAME Generic) # Generic for now, to not trigger a Warning
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_SYSTEM_PROCESSOR x86)
set(CMAKE_C_COMPILER_ID Wasienv)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER $ENV{WASI_CC})
set(CMAKE_CXX_COMPILER $ENV{WASI_CXX})
set(CMAKE_LINKER $ENV{WASI_LD})
set(CMAKE_AR $ENV{WASI_AR} )
set(CMAKE_RANLIB $ENV{WASI_RANLIB} )
include(${WASI_SDK_PREFIX}/share/cmake/wasi-sdk.cmake)
set(CMAKE_EXE_LINKER_FLAGS "-Wl")



if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Building with ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} on ${CMAKE_SYSTEM}")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


set(CMAKE_C_FLAGS "$ENV{CFLAGS} -Wall -Wextra -Wno-sign-compare -Wno-missing-field-initializers -Wundef -Wuninitialized -Wunused -Wno-unused-parameter -Wwrite-strings -Wchar-subscripts -funsigned-char")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DDEBUG -O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -DNDEBUG -O2 -g")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -D_WASI_EMULATED_SIGNAL -fno-exceptions -Wall -Wextra -Wno-sign-compare -Wno-missing-field-initializers -Wundef -Wuninitialized -Wunused -Wno-unused-parameter -Wwrite-strings -Wchar-subscripts -funsigned-char")

add_definitions(
    -DNDEBUG
)
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS},--export=__allocate_string,--export-table")

add_executable(evm
   ${CMAKE_CURRENT_SOURCE_DIR}/src/evm.cpp
)
target_link_options(evm INTERFACE -Wl,--allow-undefined,--strip-all)
target_link_libraries(evm PRIVATE boost)

target_include_directories(evm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/external
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${OPENSSL_INCLUDE_DIR}/
)


target_compile_definitions(evm PRIVATE -D_LIBCPP_STRSTREAM -DBOOST_EXCEPTION_DISABLE=1 -D_GLIBCXX_SSTREAM=1 -D_LIBCPP_SSTREAM -DBOOST_NO_IOSTREAM -DBOOST_NO_STRINGSTREAM )

target_link_options(evm PUBLIC -use-freeing-malloc)

target_link_libraries(evm PRIVATE nlohmann_json::nlohmann_json ${OPENSSL_LIBRARIES})

set_target_properties(evm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${ROOT_BINARY_DIR}"
)
