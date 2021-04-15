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



add_subdirectory(boost)
