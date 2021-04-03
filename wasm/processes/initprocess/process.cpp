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
#include "../include/libedgeos.h"
int main(int argc, const char **argv){
    // std::cout << "Hello World!";
    // char c[35];        
    // sprintf(c,"hello from process");
    elog("init process running");

    auto init_opts = json::parse(argv[0]);
    
    cbfunc_j_t onMessage = [](json * presponse ){
        json response = *presponse;
        elog("got to onMessage: " + response["command"].get<std::string>());
    };
    json p = {        
            {"nexus", init_opts["nexus"]},
            {"onMessage",(int)onMessage}
    };

    edgeos_web3_subscribe(&(p),
        [](json * presponse2){            
            json response2 = *presponse2;
            auto cid = response2["cid"].get<std::string>();
            elog("listening to contract, cid:" +cid );
        });
    
    return 0;
    
    // read from nexus
    // subscribe to nexus for changes
    
}
