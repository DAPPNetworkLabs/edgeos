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

// import all system call
// kernel main
// hook ioctl with imports
// init kernel modules

bool is_syncd(){
    // havn't seen newer confirmed block 
    return false;
}


void revert_to_block(){

}

void produce_block(){
    // select pending transactions    
    // run in vm process (start from latest block)
    // spawn or kill new chain process with new genesis (pubkeys)
    // save block contents
    // save new block header with previous header
    // propose
}
void got_block_proposal(){
    // validate block and results
    // pre-commit
}
void got_precommit(){
    // if enough precommits collected, commit
}

void got_commit(){
    // if enough commits collected, mark as final internally
    // add to real journal    
    // broadcast to other processes
    // save ipfshash to latestblock file    
    // snapshot
    // if != latest block, restore vm process state from ipfs (kill and start with latest ipfs state hash)
    // marked as syncd
    // if self produced the block, release pending transaction/callback for finality
}
bool is_producer(){
    
    return false;
}
bool is_active_producer(){
    return false;
}

void potential_block(){
    if(is_producer()){
        if(is_active_producer()){
            produce_block();            
        }        
    }    
    // edgeos_schedule(60*1000,potential_block);
}

void first_sync(){
    if(is_producer()){
        // pubsub to mempool    
        // edgeos_libp2p_subscribe("transactions",gotTransaction);
        // edgeos_libp2p_subscribe("precommits",gotPrecommit);
        // edgeos_libp2p_subscribe("blockproposals",gotBlockProposal);
        
        // every 1 minutes, potentially produce block
        // edgeos_schedule(60*1000,potential_block);
    }
    // collect messages

    // open rpc server if available
    // listen to connections and implement web3 rpc:
    // * send: verify and forward to mempool
    // * call: simulate locally without saving vm state
}


void read_genesis(){

}


//blockproposals/precommits
void start(){
    // verify messages, ban fraud messages
    // edgeos_schedule(1000*60,start);

    // pubsub to commits
    // edgeos_libp2p_subscribe(1000,got_commit);
}
int main(int argc, const char **argv){
    // restore snapshot
    const char * prm = argv[0];
    elog("chain process running: " + std::string(prm));

    // start from known verifiers
    // connect to beacons topic
    // collect messages for a minute
    return 0;
}
