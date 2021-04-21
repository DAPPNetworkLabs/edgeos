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


int main(int argc, const char **argv){
    const char * fname = argv[1];
    FILE* f = fopen(fname, "rt");
    if (!f) return 1;
    char buffer[1024];
    int size = fread(buffer, 1, sizeof(buffer), f);
    printf(">>>\\n%.*s\\n<<<", size, buffer);
    elog("test success: " + std::to_string(size) + " ");
    return 0;
}