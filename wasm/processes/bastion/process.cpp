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

#include "../include/libedgeos.h"


void timer(){
    auto p = ((json){
        {"ms", 60000},
    });
    edgeos_schedule(&p, [](json * presponse){
            elog("firing beacon");
            // broadcast dspaddress in libp2p with proof            
            timer();
        });
}
int main(int argc, const char **argv){
    // read nonce from state
    const char * from = argv[0];
    const char * to = argv[1];
    timer();
    // listen to libp2p topic for messages from owner
    // message:
    //    topic
    //    
    // dispatch ipc commands to owned processes
    // return result to sender (signed)
    return 0;
}
