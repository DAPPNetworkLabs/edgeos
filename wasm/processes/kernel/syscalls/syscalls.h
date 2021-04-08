#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <unistd.h>
#include <chrono>
#include <mutex>
#include <functional>
#include <iostream>
#include <cmath>


#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define ARSG int jsonLen, const char *json, int pidLen, const char* pidStr, void cb(int jsonResLen, const char * jsonRes,int cbcb), int cb2
typedef void (*cbfunc_t)(int jsonResLen, const char * jsonRes,int cb);
typedef void (*cbfunc_j_t)(json * jsonRes);

cbfunc_t cbfunc =
    [](int jsonResLen, const char * jsonRes, int cb){ 
            json result = (json)json::parse(jsonRes, nullptr, false); 
            if(result.is_discarded()){
                return;
            }
            auto ax = (cbfunc_j_t)cb;
            ax(&result);
        }; 
#define EDGEOS_SYSCALL(name) \
WASM_IMPORT int _edgeos_##name(ARSG)  __attribute__(( \
    __import_module__("edgeos"), \
    __import_name__(STR(name)) \
)); \
WASM_EXPORT int edgeos_syscall_##name(ARSG){\
    return _edgeos_##name(jsonLen,json,pidLen,pidStr,cb,cb2); \
}; \
int edgeos_##name(json *a,void callback(json * message)) {\
    auto jsonParams = a->dump(); \
    return _edgeos_##name( \
        jsonParams.size(), \
        jsonParams.c_str(), \
        1, \
        "0", \
        cbfunc, \
        (int)callback \
    ); \
}



/*

    auto fp = *l1.target<void*(int, const char *)>(); \

callback(jsonResLen,jsonRes); \

json * jsonRes;

    void (*ptr)(int, const char *) = &l1__##name(); \


            json result = (json)json::parse(jsonRes); \
            callback(&result);



                auto fp = *l.target<void(int, const char *)>(); \



std::function< \
        std::function< \
            void(*)(int, const char *)\
        >(void)\
> l1__##name = [](int jsonResLen, const char * jsonRes)->void (*){ \
        json result = (json)json::parse(jsonRes); \
    };\

        */