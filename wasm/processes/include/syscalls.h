#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;


#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define ARSG int jsonLen, const char *json, void cb(int jsonResLen, const char * jsonRes,int cbcb), int cb2
typedef void (*cbfunc_t)(int jsonResLen, const char * jsonRes,int cb);
typedef void (*cbfunc_j_t)(json * jsonRes);
cbfunc_t cbfunc =
    [](int jsonResLen, const char * jsonRes, int cb){ 
            json result = (json)json::parse(jsonRes); 
            auto ax = (cbfunc_j_t)cb;
            ax(&result);
        }; 

#define EDGEOS_SYSCALL(name) \
WASM_IMPORT int _edgeos_##name(ARSG) __attribute__(( \
    __import_module__("edgeos"), \
    __import_name__(STR(name)) \
)); \
int edgeos_##name(json *a,void callback(json * message)) {\
    auto jsonParams = a->dump(); \
    return _edgeos_##name( \
        jsonParams.size(), \
        jsonParams.c_str(), \
        cbfunc, \
        (int)callback \
    ); \
}

EDGEOS_SYSCALL(spawn)
EDGEOS_SYSCALL(schedule)
EDGEOS_SYSCALL(ipfs_savefs)
EDGEOS_SYSCALL(ipfs_loadfs)
EDGEOS_SYSCALL(ipfs_read)
EDGEOS_SYSCALL(ipfs_write)
EDGEOS_SYSCALL(readfile)
EDGEOS_SYSCALL(writefile)
EDGEOS_SYSCALL(web3_subscribe)
EDGEOS_SYSCALL(inter_process_call)
EDGEOS_SYSCALL(log)
void elog(std::string str){
   json j2 = {
        {"message", str},
   };

   edgeos_log(&j2, [](json * message){
       // done
   });
}
//inter_process_call
//schedule
//web3_subscribe
//web3_contract_call
//web3_contract_send

// libp2p



// edgeos_user
WASM_IMPORT void edgeos_snapshot(void (*f)(int,char *)) __attribute__((
    __import_module__("edgeos_user"),
    __import_name__("snapshot")
));

WASM_IMPORT void edgeos_restore(int strlen, const char *str, void (*f)(void)) __attribute__((
    __import_module__("edgeos_user"),
    __import_name__("restore")
));