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
#include "../include/eventFinalityManager.hpp"

bool timerSch = false;
std::string * myDSP;
struct spawn_event_t{
    std::string pid;
    std::string consumer;
    std::string processJson;
};
void spawnProcess(spawn_event_t event){
    auto evJSON = json::parse(event.processJson, nullptr, false);
    if(evJSON.is_discarded()){
        elog("discarded invalid json");
        return;
    }
    json p = {
        {"hash", evJSON["hash"]},
        {"command", evJSON["command"]},
        {"owner", event.consumer},
        {"fshash", evJSON["fshash"]},
        {"args", evJSON["args"]},
        {"pid", event.pid}
    };
    edgeos_ipfs_read(&p, [](json * result){
        // json result = (json)json::parse(jsonRes);
        // auto res = (result);
        auto res = (*result);
        auto request= res["request"];
        auto wasm = res["bytes"].get<std::string>();
        json p2 = {
            {"wasm", wasm},
            {"owner",request["owner"]},
            {"pid",request["pid"]},
            {"fshash",request["fshash"]},
            {"command",request["command"]},
            {"args",request["args"]},
            {"pid", request["pid"]}
        };
         edgeos_spawn(&p2, [](json * spawn_result){
            auto res2 = (*spawn_result);
            auto pid = res2["pid"].get<std::string>();
            elog("consumer process loaded:" + pid);
        });
    });

}
class SpawnEventFinalityManager: public EventFinalityManager<spawn_event_t>{
    public:
    SpawnEventFinalityManager():EventFinalityManager(5){
        
    }
    void scheudle();
    
    void handleEvent(spawn_event_t event, std::string id){        
        // TODO: catch kill events too
        spawnProcess(event);
    }
};

SpawnEventFinalityManager *efm = new SpawnEventFinalityManager();

void SpawnEventFinalityManager::scheudle()
{
    if(timerSch){
        return;    
    }
    timerSch = true;
    auto p = ((json){
            {"ms", 1000},
        });
    edgeos_schedule(&p, [](json * presponse){
            timerSch = false;            
            efm->popHandleEvents();
        });
}



int main(int argc, const char **argv){
    // std::cout << "Hello World!";
    // char c[35];        
    // sprintf(c,"hello from process");

    auto init_opts = json::parse(argv[0]);
    myDSP = new std::string(init_opts["dspAddress"].get<std::string>());
    elog("init process running: " + *myDSP);

    // get latest procs
    // get events since latest procs

    cbfunc_j_t onMessage = [](json * presponse ){
        json response = *presponse;
        // spawn
        
        auto removed =response["removed"].get<bool>();
        auto blockNumber = response["blockNumber"].get<long>();
        auto eventType = response["type"].get<std::string>();
        auto returnValues = response["returnValues"];
        auto id = response["id"].get<std::string>();
        auto consumer = returnValues["0"];
        auto dsp = returnValues["1"].get<std::string>();
        auto processJson = returnValues["2"].get<std::string>();
        auto pid = returnValues["3"].get<std::string>();
        if(dsp != (*myDSP) )
            return;
        spawn_event_t se;
        se.pid = pid;
        se.consumer = consumer;
        se.processJson = processJson;
        efm->addEvent(blockNumber, removed, se, id);
    };

    json p = init_opts["nexus"];
    p["onMessage"] =  (int)onMessage;
    p["eventName"] = "Spawn";
    edgeos_web3_subscribe(&(p),
        [](json * presponse2){            
            elog("listening to contract" );
        });
    
    return 0;
    
    // read from nexus
    // subscribe to nexus for changes
    
}
