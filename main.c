#include <stdio.h>

#include "LibFS.h"

void 
usage(char *prog)
{
    fprintf(stderr, "usage: %s <disk image file> I added stuff here to check\n", prog);
    exit(1);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
	usage(argv[0]);
    }
    char *path = argv[1];

    FS_Boot(path);
    FS_Sync();
    return 0;
}

