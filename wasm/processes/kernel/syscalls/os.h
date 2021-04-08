EDGEOS_SYSCALL(spawn)
EDGEOS_SYSCALL(schedule)
EDGEOS_SYSCALL(ipc_call)


WASM_EXPORT int edgeos_syscall_writefile(int pathlen, const char *pathstr,int textlen, char *text, int pid) {
    return writeFile(pathstr,text);
}
WASM_EXPORT char* edgeos_syscall_readfile(int pathlen, const char *pathstr, int pid) {
    char * c2 = (char*)malloc(MAXBUFLEN);
    if(readFile(pathstr,c2)){
        return c2;
    }
    return c2;
}


// add POSIX-security
// add pid<->user:group
// user:group,
