#include "LibFS.h"
#include "LibDisk.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// global errno value here
int osErrno;

//1st block super block, 2nd block inode bitmap, 3rd block databitmap,
//4th-128th blocks are inode blocks,
//129th to the very end are data blocks.
int inode_size = 256;
char globalPointer[512*1000];

//declare a global character
char* globalPath;

int MAX_FILES = 1000;
int MAX_FILES_COUNTER = 0;
int OPEN_FILE_TABLE_COUNTER = 0;

char* Dir_path;

enum FILETABLE { ft_int , ft_int2 , ft_filepath , ft_int3 , ft_filename};
struct FTAble{
    //enum FILETABLE;
    union {
        int entry;
        int inode;
        int *filepointer[30];
        int opencounter;
        char *file;
    };
}OPEN_FILE_TABLE[256];


// directory ops
// We made Dir_Inode_size_Add, Dir_Inode_size_Reduce, and Dir_Check for speeding up various things and we reuse various parts of code from them in the file functions


//this function takes a buffer contain parent_inode number, the original length of the parent_inode buffer, and the index of data block our new directory is in
int
Dir_Inode_Size_Add(char* parent_inode,int n ,int b)
{
    char buffer[512];
    //This function find the outmost parent inode block
    Disk_Read(((parent_inode[n]+6)/2),buffer);
    if((parent_inode[n]+6)%2==0)
    {
        buffer[0] += 1;
        //add size 1 to the outmost parent inode block
        if(n == 1)
        {//saying this is the inmost parent block
            for(int a = 2; a<256; a++)
            {//we find the first untaken space
                if(buffer[a] == -1)
                {
                    buffer[a] = b+128;//write down the block number of its child
                    break;
                }
            }
            Disk_Write((parent_inode[n]+6)/2,buffer);
        }
    }
    else
    {
        buffer[256] += 1;
        if (n == 1)
        {
            for(int a = 2+256; a<512; a++)
            {
                if(buffer[a] == -1)
                {
                    buffer[a] = b+128;
                    break;
                }
            }
        }
        
        Disk_Write((parent_inode[n]+6)/2,buffer);
    }
    n -= 1;//reduce the parent size by 1
    if(n>1)
    {//there are more than one parent left inside the buffer
        Dir_Inode_Size_Add(parent_inode, n,b);
    }
    return 0;
}


int
Dir_Inode_Size_Reduce(char* parent_inode,  int n)
{
    char buffer[512];
    //This function find the outmost parent inode block
    Disk_Read(((parent_inode[n]+6)/2),buffer);
    if((parent_inode[n]+6)%2==0)
    {
        buffer[0] -= 1;
        //subtract size 1 to the outmost parent inode block
    }
    else
    {
        buffer[256] += 1;
        Disk_Write((parent_inode[n]+6)/2,buffer);
    }
    n -= 1;//reduce the parent size by 1
    if(n>1)
    {//there are more than one parent left inside the buffer
        Dir_Inode_Size_Reduce(parent_inode, n);
    }
    return 0;
}


///Summary
/*
 
 This Dir check takes three arguments
 One is the index of slash
 One is the parent inode number
 And one is the path
 
 int x = parent inode
 int d = slash index
 path = path
 
 */


int
Dir_Check(int d,int x,char* path){
    
    
    char inode[512], dblock[512];
    
    
    //because each sector has two inode blocks, and there are 3 sectors in front of inode blocks, we find which sector our inode block is in
    int entry_start = 0;
    //declare an int to track which index in the parent datablock should we compare
    
    Disk_Read((d+6)/2,inode);
    if(d%2 == 0)
    {//this tells us our inode block start at the beginning of the sector
        Disk_Read(inode[2],dblock);
    }
    else//this means our inode block starts at the middle of the sector
    {
        Disk_Read(inode[258],dblock);
    }
    for(int i = 0; i<512; i++)
    {
        if((char)dblock[i] == '\0')
        {
            entry_start = i+2;
            break;
        }
    }
    
    int k = 0;
    while(dblock[entry_start] != 0)
    {
        if(dblock[entry_start+k] == path[k+x+1])
        {
            if((k+x+1+1) ==sizeof(path))
            {
                return dblock[entry_start + 16];
            }
            k++;
            if ((char)path[k+x+1] == '/')
            {
                return dblock[entry_start + 16];
            }
        }
        else
        {
            entry_start += 20;
        }
        
    }
    
    if(dblock[entry_start] == 0)
    {
        return -1;
    }
    return 0;
}

int
FS_Boot(char *path)
{
    globalPath = path;
    
    
    //Creating a array to store path inside, this was because we were attempting to get things working and were having trouble
    char CharacterArrayOfPath [(sizeof(path)/sizeof(char))];
    for(int i = 0; path[i] != '\0'; i++){
        CharacterArrayOfPath[i] = path[i];
    }
    
    
    // Print statements testings things
    //printf("Character Array is: %s\n", CharacterArrayOfPath);
    //printf("The path is: %s\n", path);
    //printf("Disk_load will return what: %i\n", Disk_Load(path));
    
    
    // Creating an integer for storing the return value of Disk_Load which can be used to tell if the disk can be loaded or not
    int w = 0;
    w = Disk_Load(path);
    if(w == 0){
        // If true then load the disk other and check the required information
        char rbuf[512];
        Disk_Load(path);
        if (sizeof(path) == 512*1000) {//check the disk file size
            Disk_Read(0,rbuf);
            if(rbuf[0]!= 7){//check the superblock information
                osErrno = E_GENERAL;
                return -1;
            }
        }
        else{
            osErrno = E_GENERAL;
            return -1;
        }
        
    }
    else{
        
        // More print statements for testing
        // If Disk_Load returned -1 then we need to create the disk as one doesn't exist yet
        
        //printf("We are inside the else of boot\n");
        
        // Checking to see if Disk_Init() is working corretly
        int z = 0;
        z = Disk_Init();
        printf("The disk init is: %i\n", z);
        if (Disk_Init() == -1) {
            printf("Disk_Init() failed\n");
            osErrno = E_GENERAL;
            return -1;
        }
        
        //construct superblock
        //declare a buffer has the same size with a sector for writing
        char wbuf[512];
        //The magic number we choose for super block
        wbuf[0]=7;
        //Write the super blcok
        Disk_Write(0, wbuf);
        
        printf("Disk_write is returning: %i\n", Disk_Write(0,wbuf));
        
        //initialize all of the inode blocks and give them value of -1
        char ibuf[512];//declare a buffer to use for inode block and "/" construction
        
        for(int j = 0; j<125; j++){
            ibuf[0]=0;
            ibuf[1]=0;
            for (int i=2; i<inode_size; i++){
                ibuf[i] = -1;
            }
            ibuf[0+inode_size]=0;
            ibuf[1+inode_size]=0;
            for (int k=2+inode_size; k<512; k++){
                ibuf[k] = -1;
            }
            Disk_Write(j+3,ibuf);
        }
        
        //construct directory "/" in inode
        Disk_Read(1,ibuf);
        ibuf[0]=1;
        Disk_Write(1,ibuf);//change the first inode to occupied in inode bitmap
        Disk_Read(3,ibuf);
        ibuf[0]=0;
        ibuf[1]=1;
        ibuf[2]=0;
        Disk_Write(3,ibuf);
        
        //change the first inode block, size 0 and type directory, pointing to datablock 128
        //construct directory "/" in data block
        Disk_Read(2,ibuf);
        ibuf[0]=1;
        Disk_Write(2,ibuf);//change the first datablock to occupied in databitmap
        Disk_Read(128,ibuf);
        ibuf[0] = '/';
        ibuf[1] = '\0';
        ibuf[2] = 0;
        //change the first data block, name "/" and pointing to inodeblock 0
        Disk_Save(path);//save the disk file
        Disk_Load(path);//load it again
    }
    return 0;
}


int
FS_Sync()
{
    printf("We are inside FS_Sync\n");
    printf("Global path is: %s\n", globalPath);
    if(Disk_Save(globalPath) == 0){//check if the disk save sucessfully
        if(Disk_Load(globalPath) == -1){//check if the disk load sucessfully
            osErrno = E_GENERAL;
            return -1;
        }
    }
    else{
        osErrno = E_GENERAL;
        return -1;
    }
    return 0;
}


int
File_Create(char *file)
{
    
    int slash_index = 1;
    int slash_num = 1;
    int n = 0;
    char ibit[512], inode[512],databit[512], datablock[512], parent_inode_block[512], parent_data_block[512];
    
    /// This tells is for checking the direcoty and returns # if they match and -1 if they don't
    
    for ( int i =1; i<sizeof(file); i++)
    {
        if(file[i] == '/')
        {
            if (slash_num == 0)
            {
                n = Dir_Check(slash_num,slash_index,file);
                
            }
            if(n != -1)
            {
                slash_index = i;
                slash_num += 1;
            }
            else
            {
                osErrno = E_CREATE;
                return -1;
            }
            
        }
    }
    
    
    int parent_inode = 0;
    //initialize parent inode number to be 0
    char parent_inodebuf[slash_num+1];
    //build a buffer contains all of the parent inode numbers if the layer is more than 1
    parent_inodebuf[slash_num] = 0;
    //initialize the last number of the buffer to be 0
    
    //find the parent inode number using check function
    while(slash_num>0)
    {
        int n = Dir_Check(parent_inode,slash_index, file);
        // n is the inode #
        if(n==-1) // the basic layer has not yet created
        {
            osErrno = E_CREATE;
            return -1;
        }
        else
        {
            for (int i = slash_index; i<sizeof(file); i++)
            {
                if((char)file[i+1] == '/')
                {
                    slash_index = i;
                    slash_num -= 1;
                    break;
                }
            }
            
            //when the first "/" is found, we track the inode number of the directory before this slash
            parent_inode = n;
            parent_inodebuf[slash_num] = parent_inode;
        }
    }
    
    //now the parents exist, we write the directory we asked to
    if(slash_num == 0 && MAX_FILES_COUNTER < MAX_FILES)
    {
        //find the first available inode block in inode bitmap and label it as occupied
        Disk_Read(1,ibit);
        for(int j =1; j<512; j++)
        {
            if(ibit[j] == 0)
            {
                ibit[j]=1;
                Disk_Write(1,ibit);
                
                //find the first available data block in data bitmap
                Disk_Read(2,databit);
                for(int k=0; k<512; k++)
                {
                    int x;
                    if(databit[k] == 0)
                    {
                        databit[k] = 1;//change the found data block occupied in data bitmap
                        Disk_Write(2,databit);
                        
                        //find the data block we found,first starts at 128
                        Disk_Read(k+128,datablock);
                        int x = 0;
                        while(x<sizeof(file)-slash_index-1)
                        {
                            datablock[x] = file[x+slash_index+1];
                            //write the name of the directory to datablock
                            x++;
                        }
                        
                        datablock[x+1] = '\0';
                        //write a "\0" to indicate the end of the name
                        datablock[x+2] = j;
                        //refer this datablock back to the inode block number
                        Disk_Write(k+128,datablock);
                    }
                    
                    //now write the information(entry) in the parent datablock
                    int w;
                    Disk_Read((n+6)/2,parent_inode_block);
                    for(w = 2; w < 32; w++)
                    {
                        if((n+6)%2 == 0)
                        {
                            Disk_Read((parent_inode_block[w]),parent_data_block);
                        }
                        else
                        {
                            Disk_Read((parent_inode_block[w+256]),parent_data_block);
                        }
                        int counter = 0;
                        for (int q = 0; q<512; q++)
                        {
                            if ((char)parent_data_block[q] == '\0')
                            {
                                counter = q + 2;
                                break;
                            }
                        }
                        while(parent_data_block[counter]!=0)
                        {
                            counter += 20;
                        }
                        if(counter<(512-20))
                        {
                            int y = 0;
                            while( y<(sizeof(file)-slash_index-1))
                            {
                                parent_data_block[counter+x] = file[x+slash_index+1];
                                //write the name of the directory to datablock
                                y++;
                            }
                            
                            parent_data_block[counter+y+1] =j;
                            //refer this datablock back to the inode block number
                        }
                        if(w > 32)
                        {
                            osErrno = E_CREATE;
                            return -1;
                        }
                    }
                    
                    
                    //now change the data in inode block
                    Disk_Read((j+6)/2,inode);
                    //because each sector has two inode blocks, and there are 3 sectors in front of inode blocks, we find which sector our inode block is in
                    
                    if((j+6)%2==0)//this tells us our inode block start at the beginning of the sector
                    {
                        inode[0] = 0;
                        inode[1] = 2;//1 means directory
                        inode[3] = k;
                        Disk_Write((j+6)/2,inode);
                    }
                    else//else the inode block start at the middle of the sector
                    {
                        inode[256] = 1;
                        inode[257] = 1;
                        inode[258] = k;
                        Disk_Write((j+6)/2,inode);
                    }
                    
                    //after changing everything about the inode block we now have to change the data in parent inode blocks
                    int l = sizeof(parent_inodebuf);
                    Dir_Inode_Size_Add(parent_inodebuf,l,k);
                    break;
                }
                break;
            }
        }
        MAX_FILES_COUNTER ++;
    }
    printf("FS_Create\n");
    return 0;
}



int
File_Open(char *file)
{
    int NumberOfTimesFileIsOpen = 0;
    int slash_index = 1;
    int slash_num = 1;
    int slash_num_Parent = 0;
    int FileInode = 0;
    int ParentFileInode;
    char inode[512], dblock[512], parent_data_block[512];
    //because each sector has two inode blocks, and there are 3 sectors in front of inode blocks, we find which sector our inode block is in
    int entry_start = 0;
    //declare an int to track which index in the parent datablock should we compare
    /// This is the path look up
    for ( int i =1; i<sizeof(file); i++)
    {
        if((char)file[i] == '/')
        {
            if (slash_num == 0)
            {
                FileInode = Dir_Check(slash_num,slash_index,file);
                
            }
            if(FileInode != -1)
            {
                slash_index = i;
                slash_num += 1;
                
            }
            else
            {
                osErrno = E_CREATE;
                return -1;
            }
        }
        
    }
    
    char buffer[512];
    char NameBuffer[30];
    
    // This means I am reading in the sector (n+6)/2 and putting it into a buffer called inode which is 512 in size
    Disk_Read(((FileInode+6)/2),inode);
    // This is telling it to start to read at the fourth sector of inode and store it into a buffer dblock which is 512 in size
    if((FileInode+6)%2 == 0)
    {
        Disk_Read(inode[2],dblock);
    }
    // This is looping through all 512 Inode blocks?
    for (int i = 2; i < 512; i++){
        // Inode[i] is an inode block and I am checking to see if its != -1 and if true I want to do into it
        if (inode[i] != -1){
            Disk_Read(inode[1],buffer);
            int x = 0;
            while((char)buffer[x] != '\0')
            {
                NameBuffer[x] = buffer[x];
                x++;
            }
        }
    }
    
    /// Summary
    /// This is about where we stopped writing code and switched to try to make sure everything compiled.
    /// We have almost all the information we need to add to our Open File Table that we created up at the top.
    /// Some of the comments are for keeping track of things for ourselves and reminding ourselves what everything is.
    
    
    
    
    //File table
    // first should be entry #
    // Second is the inode #
    // third is the file pointer
    // Last is if it is if the file is open or not
    
    //int entry;
    //int inode;
    //char *path;
    //int opencounter;
    //char *file;
    
    // OPEN_FILE_TABLE[i] = [i,n,path,FileWriteable];
    
    /*
     for (int i = 0; i <sizeof(OPEN_FILE_TABLE); i++){
     if (OPEN_FILE_TABLE[i].entry == i && OPEN_FILE_TABLE[i].inode != n){
     OPEN_FILE_TABLE[i].entry = i;
     OPEN_FILE_TABLE[i].inode = n;
     
     OPEN_FILE_TABLE[i].path = file;
     OPEN_FILE_TABLE[i].opencounter = i+1;
     OPEN_FILE_TABLE[i].file = file;
     
     }
     
     
     
     
     }
     */
    
    
    
    
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







int
Dir_Size(char *path)
{
    printf("Dir_Size\n");
    int parent_inode = 0;
    int slash_num = 1, slash_index = 0;
    
    //slash_num is 1 here because we are tracking the current directory, not the parent's
    char ibuf[512];
    for (int i = 1; i<sizeof(path)+1; i++)
    {
        if((char)path[i] == '/')
        {
            slash_num+=1;
        }
    }
    
    int n = 0;
    while (slash_num>0)
    {
        n = Dir_Check(parent_inode,slash_index,path);
        if(n!=-1)
        {
            for (int i = slash_index; i<sizeof(path); i++)
            {
                if((char)path[i+1] == '/')
                {
                    slash_index = i;
                    slash_num -= 1;
                    break;
                }
            }
            
            //when the first "/" is found, we track the inode number of the directory before this slash
            parent_inode = n;
        }
    }
    
    //read the directory inode block
    Disk_Read((n+6)/2,ibuf);
    if((n+6)%2 == 0)
    {
        return (ibuf[0]);//return the size if it starts at the beginning of the sector
    }
    else
    {
        return(ibuf[256]);//return the size if it starts at the middile of the sector
    }
}


int
Dir_Read(char *path, void *buffer, int size)
{
    
    //find the inode number of the path
    if(size < (Dir_Size(path)*20))
    {
        //if size is not big enough to contain enough entries
        osErrno=E_BUFFER_TOO_SMALL;
        return -1;
    }
    else
    {
        int n =0;
        int parent_inode = 0;
        int slash_num = 1, slash_index = 0;
        char ibuf[512];
        for (int i = 1; i<sizeof(path)+1; i++)
        {
            if((char)path[i] == '/')
            {
                slash_num+=1;
            }
        }
        
        while (slash_num>0)
        {
            n = Dir_Check(parent_inode,slash_index,path);
            if(n!=-1)
            {
                for (int i = slash_index; i<sizeof(path); i++)
                {
                    if((char)path[i+1] == '/')
                    {
                        slash_index = i;
                        slash_num -= 1;
                        break;
                    }
                }
                parent_inode = n;
            }
        }
        
        
        
        //read the current directory inode block
        Disk_Read((n+6)/2,ibuf);
        int counter = 0;//use to detect the correct place in buffer
        char databuf[512];
        char buffer[512];
        if((n+6)%2==0)
        {
            for(int j = 2; j<256; j++)
            {
                if (ibuf[j]!=-1)
                {
                    Disk_Read(ibuf[j],databuf);
                    for(int k = 0; k<512; k++)
                    {
                        buffer[counter+k] = databuf[k];
                        if((char)databuf[k+1] == '\0')
                        {
                            buffer[counter+16] = databuf[k+2];
                            counter+=20;
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            for(int j = 258; j<512; j++)
            {
                if (ibuf[j]!=-1)
                {
                    Disk_Read(ibuf[j],databuf);
                    for(int k = 0; k<512; k++)
                    {
                        buffer[counter+k] = databuf[k];
                        if((char)databuf[k+1] == '\0')
                        {
                            buffer[counter+16] = databuf[k+2];
                            counter+=20;
                            break;
                        }
                    }
                }
            }
        }
        
    }
    return 0;
}

int
Dir_Create(char *path)
{
    
    char ibit[512], inode[512],databit[512], datablock[512], parent_inode_block[512], parent_data_block[512];
    //declare a buffer for inode bitmap and inode block and datablock to read and write
    
    int slash_num =0,slash_index=0;
    //declare an int to track which layer of directory has been created, and where the previous slash is
    
    //find how many levels of directory are in the path
    for (int i = 1; i<sizeof(path)+1; i++)
    {
        if((char)path[i] == '/')
        {
            slash_num+=1;
        }
    }
    
    int parent_inode = 0;
    //initialize parent inode number to be 0
    char parent_inodebuf[slash_num+1];
    //build a buffer contains all of the parent inode numbers if the layer is more than 1
    parent_inodebuf[slash_num] = 0;
    //initialize the last number of the buffer to be 0
    
    int n = 0;
    
    //find the parent inode number using check function
    while(slash_num>0)
    {
        n = Dir_Check(parent_inode,slash_index,path);
        // n is the inode #
        if(n==-1) // the basic layer has not yet created
        {
            osErrno = E_CREATE;
            return -1;
        }
        else
        {
            for (int i = slash_index; i<sizeof(path); i++)
            {
                if((char)path[i+1] == '/')
                {
                    slash_index = i;
                    slash_num -= 1;
                    break;
                }
            }
            
            //when the first "/" is found, we track the inode number of the directory before this slash
            parent_inode = n;
            parent_inodebuf[slash_num] = parent_inode;
        }
    }
    
    //now the parents exist, we write the directory we asked to
    if(slash_num == 0 && MAX_FILES_COUNTER < MAX_FILES)
    {
        //find the first available inode block in inode bitmap and label it as occupied
        Disk_Read(1,ibit);
        for(int j =1; j<512; j++)
        {
            if(ibit[j] == 0)
            {
                ibit[j]=1;
                Disk_Write(1,ibit);
                
                //find the first available data block in data bitmap
                Disk_Read(2,databit);
                for(int k=0; k<512; k++)
                {
                    int x = 0;
                    if(databit[k] == 0)
                    {
                        databit[k] = 1;//change the found data block occupied in data bitmap
                        Disk_Write(2,databit);
                        
                        //find the data block we found,first starts at 128
                        Disk_Read(k+128,datablock);
                        x = 0;
                        while(x<sizeof(path)-slash_index-1)
                        {
                            datablock[x] = path[x+slash_index+1];
                            //write the name of the directory to datablock
                            x++;
                        }
                        
                        datablock[x+1] = '\0';
                        //write a "\0" to indicate the end of the name
                        datablock[x+2] =j;
                        //refer this datablock back to the inode block number
                        Disk_Write(k+128,datablock);
                    }
                    
                    //now write the information(entry) in the parent datablock
                    Disk_Read((n+6)/2,parent_inode_block);
                    for(int w = 2; w<32; w++)
                    {
                        if((n+6)%2 == 0)
                        {
                            Disk_Read((parent_inode_block[w]),parent_data_block);
                        }
                        else
                        {
                            Disk_Read((parent_inode_block[w+256]),parent_data_block);
                        }
                        int counter = 0;
                        for (int q = 0; q<512; q++)
                        {
                            if ((char)parent_data_block[q] == '\0')
                            {
                                counter = q + 2;
                                break;
                            }
                        }
                        while(parent_data_block[counter]!=0)
                        {
                            counter += 20;
                        }
                        if(counter<(512-20))
                        {
                            int y = 0;
                            while( y<(sizeof(path)-slash_index-1))
                            {
                                parent_data_block[counter+x] = path[x+slash_index+1];
                                //write the name of the directory to datablock
                                y++;
                            }
                            
                            parent_data_block[counter+y+1] =j;
                            //refer this datablock back to the inode block number
                        }
                        if(w > 32)
                        {
                            osErrno = E_CREATE;
                            return -1;
                        }
                    }
                    
                    
                    //now change the data in inode block
                    Disk_Read((j+6)/2,inode);
                    //because each sector has two inode blocks, and there are 3 sectors in front of inode blocks, we find which sector our inode block is in
                    
                    if((j+6)%2==0)//this tells us our inode block start at the beginning of the sector
                    {
                        inode[0] = 0;
                        inode[1] = 1;//1 means directory
                        inode[3] = k;
                        Disk_Write((j+6)/2,inode);
                    }
                    else//else the inode block start at the middle of the sector
                    {
                        inode[256] = 1;
                        inode[257] = 1;
                        inode[258] = k;
                        Disk_Write((j+6)/2,inode);
                    }
                    
                    //after changing everything about the inode block we now have to change the data in parent inode blocks
                    int l = sizeof(parent_inodebuf);
                    Dir_Inode_Size_Add(parent_inodebuf,l,k);
                    break;
                }
                break;
            }
        }
        MAX_FILES_COUNTER ++;
    }
    printf("Dir_Create %s\n", path);
    return 0;
}



int
Dir_Unlink(char *path)
{
    int n = 0;
    int parent_inode = 0;
    int slash_num = 1, slash_index = 0;
    char ibuf[512];
    //char parent_inodebuf[];
    
    for (int i = 1; i<sizeof(path)+1; i++)
    {
        if((char)path[i] == '/')
        {
            slash_num+=1;
        }
    }
    char parent_inodebuf[slash_num];
    
    
    if(Dir_Size(path) < (Dir_Size(path)*20))
    {//if size is not big enough to contain enough entries
        osErrno=E_BUFFER_TOO_SMALL;
        return -1;
    }
    else
    {
        while (slash_num>0)
        {
            n = Dir_Check(parent_inode,slash_index,path);
            if(n!=-1)
            {
                for (int i = slash_index; i<sizeof(path); i++)
                {
                    
                    if((char)path[i+1] == '/')
                    {
                        slash_index = i;
                        slash_num -= 1;
                        break;
                    }
                }
                parent_inode = n;
                parent_inodebuf[slash_num] = parent_inode;
            }
        }
        
        if(parent_inode == 0)
        {
            osErrno = E_ROOT_DIR;
            return -1;
        }
        Disk_Read((n+6)/2,ibuf);
        char databuf[512],additionalbuf[512];
        int x = 0;
        if((n+6)%2 == 0)
        {
            for(int j =2; j < 256; j++)
            {
                if(ibuf[j]!=-1)
                {
                    Disk_Read(ibuf[j],databuf);
                    for(int k = 0; k<512; k++)
                    {
                        if((char)databuf[k] == '\0')
                        {
                            x = databuf[k+1];
                            Disk_Read((x+6)/2,additionalbuf);
                            if((x+6)%2 == 0)
                            {
                                for(int r =0; r<256; r++)
                                {
                                    additionalbuf[r] = -1;
                                }
                            }
                            else
                            {
                                for(int r =256; r<512; r++)
                                {
                                    additionalbuf[r] = -1;
                                }
                            }
                            Disk_Write((x+6/2),additionalbuf);
                            Disk_Read(1,additionalbuf);
                            additionalbuf[x] = 0;
                            Disk_Write(1,additionalbuf);
                        }
                        databuf[k]=0;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            for(int j =258; j < 512; j++)
            {
                if(ibuf[j]!=-1)
                {
                    Disk_Read(ibuf[j],databuf);
                    for(int k = 0; k<512; k++)
                    {
                        if((char)databuf[k] == '\0')
                        {
                            x = databuf[k+1];
                            Disk_Read((x+6)/2,additionalbuf);
                            if((x+6)%2 == 0)
                            {
                                for(int r =0; r<256; r++)
                                {
                                    additionalbuf[r] = -1;
                                }
                            }
                            else
                            {
                                for(int r =256; r<512; r++)
                                {
                                    additionalbuf[r] = -1;
                                }
                            }
                            Disk_Write((x+6/2),additionalbuf);
                            Disk_Read(1,additionalbuf);
                            additionalbuf[x] = 0;
                            Disk_Write(1,additionalbuf);
                        }
                        databuf[k]=0;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        int l = sizeof(parent_inodebuf);
        Dir_Inode_Size_Reduce(parent_inodebuf,l);
        Disk_Write((n+6)/2,ibuf);
    }
    Disk_Read((n+6)/2,ibuf);
    if((n+6)%2 == 0)
    {
        if(ibuf[0] != 0)
        {
            osErrno = E_DIR_NOT_EMPTY;
            return -1;
        }
    }
    else
    {
        if(ibuf[256] != 0)
        {
            osErrno = E_DIR_NOT_EMPTY;
            return -1;
        }
    }
    
    printf("Dir_Unlink\n");
    return 0;
}