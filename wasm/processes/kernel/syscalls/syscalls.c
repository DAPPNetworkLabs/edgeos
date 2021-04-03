#include "../../include/fsutils.h"
#include "syscalls.h"
#include "log.h"
#include "ipfs.h"
#include "web3.h"
#include "os.h"



WASM_EXPORT char *__allocate_string(int len)
{
    char *s =  (char*)malloc(len);

    return s;
}



