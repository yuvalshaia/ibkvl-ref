#include <stdio.h>
#include <stdlib.h>

#include <mlxkvl/kvl_usr.h>
#include "common.h"


int main(){
    int fd;
    struct user_st cmd;
    int res=0;
    cmd.integ = 1112;
    cmd.str= "HIIII world";
    
    if ((fd=kvl_dev_open("user_run"))<0)
        return 1;
    res = write(fd, &cmd, sizeof(cmd));
    if (res != sizeof(cmd)) {
        printf("failed to send test props to kernel level\n");
        return 1;
    }

    kvl_dev_close(fd);
    return 0;
}

