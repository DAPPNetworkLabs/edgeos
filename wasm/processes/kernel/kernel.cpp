#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syscall.h>
#include <iostream>
#include "../include/edgeos.h"
#include "syscalls/syscalls.c"

void spawnInitProcess(json init_opts){  
    auto ipfshash = init_opts["wasm"].get<std::string>();
    elog("init process loading from:" + ipfshash);
    json p = {
        {"hash", ipfshash},
        {"initOpts",init_opts}
    };
    

    edgeos_ipfs_read(&p, [](json * result){
        // json result = (json)json::parse(jsonRes);
        // auto res = (result);
        auto res = (*result);
        auto wasm = res["bytes"].get<std::string>();        
        std::string opts = res["request"]["initOpts"].dump();
        elog("loaded process from ipfs: " + std::to_string(wasm.size()));
        json p2 = {
            {"wasm", wasm},
            {"pid", 1},
            {"fshash",""},
            {"owner","system"},
            {"command",""},
            {"args", {opts}},
        };        
         edgeos_spawn(&p2, [](json * spawn_result){
            auto res2 = (*spawn_result);
            auto pid = res2["pid"].get<std::string>();
            elog("init process loaded:" + pid);
        });
    });
    
}

// kernel main
int main(int argc, const char **argv){
    std::cout << "Hello World!";
    auto init_opts = json::parse(argv[0]);

    spawnInitProcess(init_opts);
    
    // todo: init kernel modules    
    return 0;
}
