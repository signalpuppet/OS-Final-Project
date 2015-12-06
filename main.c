#include <stdio.h>
#include "LibFS.h"


void
usage(char *prog)
{
    fprintf(stderr,"usage: %s <disk image file> I am inside usages!!!!!\n", prog);
    exit(1);
}

int
main(int argc, char *argv[])
{
    argv[1] = "Documents/Operating Systems/Final_Project/Test";
    argc++;
    
    if (argc != 2) {
        usage(argv[0]);
    }
    
    char *path = argv[1];
    
    FS_Boot(path);
    FS_Sync();
    
    return 0;
}
