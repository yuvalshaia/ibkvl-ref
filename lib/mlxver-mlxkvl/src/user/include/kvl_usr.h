#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      /* for open */
#include <unistd.h>     /* for close */
#include <errno.h>      /* for errno */
#include <string.h>

#ifndef __KVL_USER__
#define __KVL_USER__

static const char* kvl_procfs = "/proc/mlxkvl";

inline int kvl_dev_open(char* kvl_dev_name){
    char procfs_dev[256];
    int fd;
    sprintf(procfs_dev,"%s/%s",kvl_procfs,kvl_dev_name);
    printf("Opening mlxkvl module %s : ",procfs_dev);
    fd = open(procfs_dev, O_RDWR);
    if (fd<0) {
        printf("Cannot open device '%s': %s\n", procfs_dev, strerror(errno));
    }else
        printf("Success fd=%d\n",fd);
    return fd;
}

inline int kvl_dev_close(int fd){
    printf("Closing mlxkvl module fd=%d\n",fd);
    return close(fd);
}

#endif
