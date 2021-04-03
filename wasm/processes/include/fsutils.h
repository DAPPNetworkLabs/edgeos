#pragma once
#define MAXBUFLEN 10000000

int readFile(const char * path, char * fileData){
  int fd = open(path, O_RDONLY | O_EXCL);
  if (fd < 0)
    return -1;
  size_t newLen = read(fd, fileData, MAXBUFLEN);  
  fileData[++newLen] = '\0'; /* Just to be safe. */

  close(fd);

  return 0;
}


int writeFile(const char * path, const char * eventData){
    ssize_t n, m;
    
    int out = open(path, O_TRUNC | O_WRONLY | O_CREAT, 0660);
    if(out < 0){
        fprintf(stderr, "open error: %s\n", strerror(errno));
        return -2;
    }
    n = strlen(eventData);
    const char *ptr = eventData;
    while (n > 0) {
        m = write(out, ptr, (size_t)n);
        if (m < 0) {
            fprintf(stderr, "write error: %s\n", strerror(errno));
            return -3;
            // exit(1);
        }
        n -= m;
        ptr += m;
    }
    close(out);
    return 0;
}