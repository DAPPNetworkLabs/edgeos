project(solidity)

set(CMAKE_SOL "/usr/bin/solc")
   

macro(sol paramTarget paramSource )
    add_custom_target("${paramTarget}" ALL  
            COMMAND "${CMAKE_SOL}" 
        "-o" 
        "${ROOT_BINARY_DIR}" 
        "--optimize" 
        "--abi" 
        "--bin"
        "--overwrite"
        "--allow-paths"
        "/"
        "zeppelin-contracts=${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/zeppelin-contracts"
        "${CMAKE_CURRENT_SOURCE_DIR}/${paramSource}.sol"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${paramSource}.sol"
        
   VERBATIM  )
endmacro(sol)

sol(nexus "nexus/nexus")
sol(evmtokenpeg "evmtokenpeg/evmtokenpeg")


# https://etclabscore.github.io/evm-llvm-website/building-evm-llvm