include(FetchContent)
include(ExternalProject)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG develop)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)

  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(microecc
  GIT_REPOSITORY https://github.com/kmackay/micro-ecc.git
  GIT_TAG master)

FetchContent_GetProperties(microecc)
if(NOT micro-ecc_POPULATED)
  FetchContent_Populate(microecc)


endif()


FetchContent_Declare(llvmwasi
  GIT_REPOSITORY https://github.com/binji/wasi-sdk-bin.git
  GIT_TAG master)
FetchContent_GetProperties(llvmwasi)
if(NOT llvmwasi_POPULATED)
  #FetchContent_Populate(llvmwasi)

endif()


FetchContent_Declare(llvm
  GIT_REPOSITORY https://github.com/binji/llvm-project.git
  GIT_TAG master)

FetchContent_GetProperties(llvm)
if(NOT llvm_POPULATED)
  #FetchContent_Populate(llvm)


endif()

#ExternalProject_Add(llvm
#      SOURCE_DIR ${llvm_SOURCE_DIR}/llvm
#      BINARY_DIR llvm
#      INSTALL_COMMAND ""
#      BUILD_ALWAYS 1
#      TEST_EXCLUDE_FROM_MAIN 1
#      CMAKE_ARGS
#          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
#          -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/../cmake/toolchain.cmake
#          -DWASI_SDK_PREFIX=${llvmwasi_SOURCE_DIR}
#          -DCMAKE_SYSROOT=${llvmwasi_SOURCE_DIR}/share/wasi-sysroot
#          -DLLVM_TARGET_ARCH=WASM
#          -DLLVM_TARGETS_TO_BUILD=""
#          -DLLVM_DEFAULT_TARGET_TRIPLE=${triple}
#          -DCMAKE_CROSSCOMPILING=True
#          -DLLVM_INCLUDE_BENCHMARKS=OFF
#          -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WASI
#          -DLLVM_ENABLE_PIC=OFF
#          -DLLVM_ENABLE_ASSERTIONS=OFF
#          -DLLVM_ENABLE_THREADS=OFF
#          -DLLVM_BUILD_LLVM_DYLIB=OFF
#          -DLLVM_BUILD_TOOLS=OFF
#          -DLLVM_ENABLE_BACKTRACES=OFF
#          -DLLVM_ENABLE_DUMP=OFF
#          -DLLVM_TABLEGEN=/usr/bin/llvm-tblgen-11
#          -DCLANG_TABLEGEN=/usr/bin/clang-tblgen-11)

add_subdirectory(boost)
add_subdirectory(memfs)
