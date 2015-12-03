#include "LibFS.h"
#include "LibDisk.h"

// global errno value here
int osErrno;

int 
FS_Boot(char *path)
{
    // Order of things
    // First check to see if disk exists
        // If it exists then use Disk_load()
        // Then check to make sure that it is a valid disk, the file size should be equal to NUM_SECTORS x SECTOR_SIZE.
        // All superblock info should be as described
            // If any of this is wrong it should fail
    
    // If disk doesn't exist
        // Create a new disk, init its superblock and other structs, and create empty root directory
        // In this case use Disk_Init()
        // Then a few Disk_Write()
        // Then a Disk_Save()
        
     // something
    
    //test
    
    
    printf("FS_Boot %s\n", path);

    // oops, check for errors
    if (Disk_Init() == -1) {
	printf("Disk_Init() failed\n");
	osErrno = E_GENERAL;
	return -1;
    }

    // do all of the other stuff needed...
    
    
    
    
    
    return 0;
}

int
FS_Sync()
{
    printf("FS_Sync\n");
    return 0;
}


int
File_Create(char *file)
{
    printf("FS_Create\n");
    return 0;
}

int
File_Open(char *file)
{
    printf("FS_Open\n");
    return 0;
}

int
File_Read(int fd, void *buffer, int size)
{
    printf("FS_Read\n");
    return 0;
}

int
File_Write(int fd, void *buffer, int size)
{
    printf("FS_Write\n");
    return 0;
}

int
File_Seek(int fd, int offset)
{
    printf("FS_Seek\n");
    return 0;
}

int
File_Close(int fd)
{
    printf("FS_Close\n");
    return 0;
}

int
File_Unlink(char *file)
{
    printf("FS_Unlink\n");
    return 0;
}


// directory ops
int
Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);
    return 0;
}

int
Dir_Size(char *path)
{
    printf("Dir_Size\n");
    return 0;
}

int
Dir_Read(char *path, void *buffer, int size)
{
    printf("Dir_Read\n");
    return 0;
}

int
Dir_Unlink(char *path)
{
    printf("Dir_Unlink\n");
    return 0;
}

