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
    auto manifest = init_opts["manifest"];
    elog("init process loading");

        json p2 = {
            {"pid", 1},
            {"fshash",manifest["initWASM"].get<std::string>()},
            {"owner","system"},
            {"command",""},
            {"args", {init_opts.dump()}},
        };        
         edgeos_spawn(&p2, [](json * spawn_result){
            auto res2 = (*spawn_result);
            auto pid = res2["pid"].get<std::string>();
            elog("init process loaded:" + pid);
        });
    
}

// kernel main
int main(int argc, const char **argv){
    std::cout << "Hello World!";
    auto init_opts = json::parse(argv[0]);

    spawnInitProcess(init_opts);
    
    return 0;
}
