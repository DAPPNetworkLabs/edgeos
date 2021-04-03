project(nexus)
cmake_minimum_required(VERSION 3.16)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( nexus nexus ${CMAKE_SOURCE_DIR}/nexus/nexus.cpp )
add_contract( Token Token ${CMAKE_SOURCE_DIR}/Token/Token.cpp )
add_contract( gatehouse gatehouse ${CMAKE_SOURCE_DIR}/gatehouse/gatehouse.cpp )
add_contract( ethtokenpeg ethtokenpeg ${CMAKE_SOURCE_DIR}/ethtokenpeg/ethtokenpeg.cpp )
target_include_directories( nexus PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_include_directories( gatehouse PUBLIC ${CMAKE_SOURCE_DIR}/../include )