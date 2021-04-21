#include <nlohmann/json.hpp>
using json = nlohmann::json;


#define WASM_EXPORT __attribute__((visibility("default")))  extern "C"
#define WASM_IMPORT extern "C"

WASM_EXPORT char *__allocate_string(int len)
{
    char *s =  (char*)malloc(len);

    return s;
}

#include "syscalls.h"
