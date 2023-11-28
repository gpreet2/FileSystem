/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
* File: mfs.c
*
* Description: directory operations
*               
*
**************************************************************/

#include "mfs.h"
#include "b_io.h"
#include "fsLow.h"
#include "dirInit.h"
#include "vcb.h"
#include "mapping.h"
#include "extTable.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//extern Global variables from mfs.h

char* cwdPath;
dirEntry* cwdEntries;


//Helper Functions

//Load the dir in the dir entries from disk
void loadDirEntries(dirEntry* DEArray, int location){
    LBAread(DEArray, DIRECTORY_BLOCKSIZE, location);
}
//helper routine 
void initGlobalVar(){
    cwdPath = malloc(256);
    strcpy(cwdPath, "/");

    cwdEntries = malloc(MAX_DIRENT_SIZE*DE_STRUCT_SIZE);
    char* DEBuffer = malloc(DIRECTORY_BLOCKSIZE*512);
    LBAread(DEBuffer, DIRECTORY_BLOCKSIZE, vcb.RootDir);
    memcpy(cwdEntries, DEBuffer, MAX_DIRENT_SIZE*DE_STRUCT_SIZE);
    
}
//helper routine 
void freeGlobalVar(){
    free(cwdEntries);
    free(cwdPath);
}

//function  gets last element within a path

char * getLastPathElement(const char *pathname){

    char *str;
    
    str = strrchr(pathname, '/');
    
    if (str != NULL) {
        str++; 
    }

    return str;
}

//function that takes a path and returns the path but not last element
char * getParentDirectory(const char *pathname){
    if(pathname[0] == '/' && strlen(pathname) <= 1){
        char* path = malloc(sizeof(char)*2);
        strcpy(path, pathname);
        return path;
    }

    char* lastElement = getLastPathElement(pathname);
    
    char tempPath[strlen(pathname)+1];
    strcpy(tempPath, pathname);

    
    int len = strlen(lastElement);
    
    if(strlen(pathname) == len+1){
        tempPath[strlen(pathname) - len] = '\0';
    }else{
        
        tempPath[strlen(pathname) - len - 1] = '\0';
    }
   
    char* path = malloc(strlen(tempPath) + 1);
    memcpy(path, tempPath, strlen(tempPath) + 1);
    
    return path;

}





//function to parse a pathname to check if valid.

pathInfo* parsePath(const char *pathname)
{
    pathInfo* result = malloc(sizeof(pathInfo));
    result->DEPointer = malloc(sizeof(dirEntry));

    int entryIndex = 0;
    char *delim = "/";
    char tempPath[256];
    tempPath[0] = '\0';
    strncpy(tempPath, pathname, strlen(pathname) + 1);
    
    dirEntry* tempDirEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
    char * DEBuffer = malloc(DIRECTORY_BLOCKSIZE*512);

    
    
   
    if (pathname[0] != '/')
    {
        strcpy(tempPath, cwdPath);
        strcat(tempPath, "/");
        strncat(tempPath, pathname, strlen(pathname));    
    }
    
    loadDirEntries(tempDirEntries, vcb.RootDir);  
    
    
    char *token = strtok(tempPath, delim);
    char *pathTokens[64];
    pathTokens[0] = NULL;

    int tokenIndex = 0;
    
    while(token != NULL){
        if(strcmp(token, "..") == 0){
            if(tokenIndex == 0){
                
            }else{
                
                tokenIndex--;
                pathTokens[tokenIndex] = NULL;
            }
                
        }else if(strcmp(token, ".") == 0){
            
        }else{
            
            pathTokens[tokenIndex] = token;
            tokenIndex++;
        }
        token = strtok(NULL, delim);
        }
    
    if(tokenIndex == 0){
        
        char* temp = ".";
        pathTokens[0] = temp;
        tokenIndex++;
        
    }
    pathTokens[tokenIndex] = NULL;
    
    
    char* absolutePath = malloc(64);
    absolutePath[0] = '\0';
    for(int i = 0; i < tokenIndex; i++){
        if(strcmp(pathTokens[0], ".") == 0){
            strcat(absolutePath, delim);
        }else{
            
            strcat(absolutePath, delim);
            strcat(absolutePath, pathTokens[i]);
        }
    }
    
    strcpy(result->path, absolutePath);
    free(absolutePath);
    
    
    int exists = 0;
    int tokenCounter = 0;

   
    while(pathTokens[tokenCounter] != NULL){
        
        for(int i = 0; i < MAX_DIRENT_SIZE; i++){
            
            if(tempDirEntries[i].dirType != -1
             && strcmp(tempDirEntries[i].name, pathTokens[tokenCounter]) == 0){
                exists = 1;
                entryIndex = i;
                
                i = MAX_DIRENT_SIZE;
            }

        }

        if(exists == 0){
            

            if(tokenCounter != tokenIndex - 1){
                entryIndex = -2;
                result->DEPointer = NULL;
                break;
            }else{
                
                entryIndex = -1;
                result->DEPointer = NULL;
                break;
            }
        }else if(exists == 1){
            
            if(tokenCounter != tokenIndex - 1){
                //Load the next directory DE
                loadDirEntries(tempDirEntries, tempDirEntries[entryIndex].location);
                exists = 0;
                tokenCounter++;
            }else{
                
                
                strcpy(result->DEPointer->name, tempDirEntries[entryIndex].name);
                result->DEPointer->created = tempDirEntries[entryIndex].created;
                result->DEPointer->size = tempDirEntries[entryIndex].size;
                result->DEPointer->dirType = tempDirEntries[entryIndex].dirType;
                result->DEPointer->extentLocation = tempDirEntries[entryIndex].extentLocation;
                result->DEPointer->lastModified = tempDirEntries[entryIndex].lastModified;
                result->DEPointer->location = tempDirEntries[entryIndex].location;

                break;
            }
        }

    }
    
    
    free(tempDirEntries);
    free(DEBuffer);

    result->value = entryIndex;
  
    return result;

}



fdDir * fs_opendir(const char *pathname){

    

    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);

    
    if(pi->value >= 0){
        if(pi->DEPointer->dirType != 1){
            printf("Not a directory\n");
            return NULL;
        }
        fdDir* fd = malloc(sizeof(fdDir));

        //initializing the fd struct elements once the dir is open
        fd->dirPointer = malloc(MAX_DIRENT_SIZE*DE_STRUCT_SIZE);
        
        loadDirEntries(fd->dirPointer, pi->DEPointer->location);
        fd->d_reclen = sizeof(fdDir);
        fd->directoryStartLocation = pi->DEPointer->location;
        fd->dirEntryPosition = 0;
        fd->dirSize = (fd->dirPointer[0].size)/DE_STRUCT_SIZE;
        fd->fileIndex = 0;

        
        free(pi->DEPointer);
        free(pi);

        return fd;  //return the directory descriptor
    }else{
        
        free(pi->DEPointer);
        free(pi);
        return NULL;
    }
}


//----------------------fs_readDir------------------------------

struct fs_diriteminfo *fs_readdir(fdDir *dirp){
    //malloc the return struct
    struct fs_diriteminfo* ii = malloc(sizeof(struct fs_diriteminfo));
    int exist = 0;

    if(dirp->fileIndex == dirp->dirSize){
        return NULL;
    }
    //loop throu each element
    for(int i = dirp->dirEntryPosition; i < MAX_DIRENT_SIZE; i++){
        if(dirp->dirPointer[i].dirType == 0 || dirp->dirPointer[i].dirType == 1){
            strcpy(ii->d_name, dirp->dirPointer[i].name);
            
            ii->d_reclen = (int) sizeof(struct fs_diriteminfo);
            if(dirp->dirPointer[i].dirType == 1){
                ii->fileType = '1';
            }else{
                ii->fileType = '0';
            }
            
            dirp->fileIndex++;
            
            
            i = MAX_DIRENT_SIZE;
            exist = 1;
        }
        dirp->dirEntryPosition++;
    }
    if(exist == 0){
        ii = NULL;
    }

    return ii;
}
//--------------------fs_closedir-------------------
int fs_closedir(fdDir *dirp){
    
    free(dirp->dirPointer);
    free(dirp);
    dirp = NULL;
}

//----------------fs_getcwd-----------------------------
char *fs_getcwd(char *pathname, size_t size)
{
    
    
    strncpy(pathname,cwdPath,size);//copy the pathname from cwdPath
    return pathname;
}

//current working directory
int fs_setcwd(char *pathname)
{
    
    LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

    if(strcmp(pathname, "./") == 0 ){

        return 0;
    }
    
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);
    
    if(pi->value >= 0){ 
        if(pi->DEPointer->dirType != 1){
            return -1;
        }
        //setting cwdEntries
        
        
        loadDirEntries(cwdEntries, pi->DEPointer->location);
        
        //setting cwdPath   
        strcpy(cwdPath,pi->path);
        
        free(pi->DEPointer);
        free(pi);
        //success
        return 0;
    }
    free(pi->DEPointer);
    free(pi);
    //fails
    return -1;
}



int fs_mkdir(const char *pathname, mode_t mode)
{
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);
    
    if(pi->value != -1){
        return -1;
    }
    char *parentPath = getParentDirectory(pi->path);
    
    // Get the last element in the path
    char *lastElementOfPath = getLastPathElement(pi->path);
    
    
    if (fs_setcwd(parentPath) == -1)
    {
        printf("Error, failed to set parent path as current working directory.");
        return -1;
    }

    

    int numOfDirEntries = MAX_DIRENT_SIZE;
    int indexOfNewDirEntry;

    for (int i = 0; i < numOfDirEntries; i++)
    {
        // if the dir is free, begin creating the new directory.
        if (cwdEntries[i].dirType == -1)
        {   
            //Initializing new directory entry
            indexOfNewDirEntry = i;
            strcpy(cwdEntries[i].name, lastElementOfPath);
            cwdEntries[i].dirType = 1;
            cwdEntries[i].size = DE_STRUCT_SIZE*2;
            cwdEntries[i].location =
                getConsecFreeSpace(vcb.freeSpaceBitMap, vcb.bitMapByteSize, DIRECTORY_BLOCKSIZE);
            updateBitMap(vcb.freeSpaceBitMap);
            time(&cwdEntries[i].created);
            time(&cwdEntries[i].lastModified);
            cwdEntries[0].size += DE_STRUCT_SIZE;
            //Update .. 
            if(cwdEntries[0].location == cwdEntries[1].location){
                cwdEntries[1].size += DE_STRUCT_SIZE;
            }else{
                //Update the parent directory 
                char* parentDir = getLastPathElement(cwdPath);
                dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
                LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
                //Locate the new directory name 
                for(int i = 0; i < MAX_DIRENT_SIZE; i++){
                    if(strcmp(parentDir, tempDEntries[i].name) == 0){
                        
                        tempDEntries[i].size += DE_STRUCT_SIZE;
                        //Exit loop
                        i = MAX_DIRENT_SIZE;
                    }
                }
                //Write back fd
                LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
                free(tempDEntries);
            }
            //Exit loop
            i = numOfDirEntries;
        }
    }
    
    
    // Parse path on passed path
    dirEntry *dirEntries = malloc(MAX_DIRENT_SIZE * DE_STRUCT_SIZE);
    loadDirEntries(dirEntries, cwdEntries[indexOfNewDirEntry].location);
   
    for (int i = 0; i < numOfDirEntries; i++)
    {
        dirEntries[i].name[0] = '\0';
        dirEntries[i].dirType = -1; // free state
        dirEntries[i].size = 0;
        dirEntries[i].location = -1;
        dirEntries[i].extentLocation = -1;
    }

    // "." Directory Entry
    strcpy(dirEntries[0].name, ".");
    dirEntries[0].size = DE_STRUCT_SIZE*2;;
    // 1 for Directory 
    dirEntries[0].dirType = 1;
    dirEntries[0].location = cwdEntries[indexOfNewDirEntry].location;
    
    time(&dirEntries[0].created);
    time(&dirEntries[0].lastModified);

    //".." Directory Entry
    strcpy(dirEntries[1].name, "..");
    dirEntries[1].size = cwdEntries[0].size;
    
    dirEntries[1].dirType = cwdEntries[0].dirType;
    dirEntries[1].location = cwdEntries[0].location;
    
    dirEntries[1].created = cwdEntries[0].created;
    dirEntries[1].lastModified = cwdEntries[0].lastModified;

    
    LBAwrite(dirEntries, DIRECTORY_BLOCKSIZE, dirEntries[0].location);
    LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

    
    free(dirEntries);
    free(pi->DEPointer);
    free(pi);

    return 0;
}

//---------------isFile----------------------

int fs_isFile(char * filename)
{
    
   
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(filename);
    
    if(pi->value >= 0)
    {   
        
        if(pi->DEPointer->dirType == 0)
        {   
            free(pi->DEPointer);
            free(pi);      
            return 1;
        }
        else {
            free(pi->DEPointer);
            free(pi);
            return 0;
        }
    } else {
        free(pi->DEPointer);
        free(pi);
        return 0;
    }
    
    
}


//------------- fs_isDir --------------

int fs_isDir(char * pathname)
{   
    
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);

    if(pi->value >= 0)
    {   
        
        if(pi->DEPointer->dirType == 1)
        {       
            free(pi->DEPointer);
            free(pi);    
            return 1;
        }
        else {
            free(pi->DEPointer);
            free(pi);
            return 0;
        }
    } else {
        free(pi->DEPointer);
        free(pi);
        return 0;
    }

}


//----------------fs_stat()----------------
int fs_stat(const char *path, struct fs_stat *buf)
{
    
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(path);
    
    
    if (pi->value >= 0){
        
        buf->st_accesstime = pi->DEPointer->lastModified;
        buf->st_size = pi->DEPointer->size;
        buf->st_createtime = pi->DEPointer->created;
        buf->st_modtime = pi->DEPointer->lastModified;
        free(pi->DEPointer);
        free(pi);
        return 1; //success
    }
    //Free malloc
    free(pi->DEPointer);
    free(pi);
    return -1;   //on failure
}

//--------------------fs_rmdir---------------------------------
int fs_rmdir(const char *pathname){
    
    if(pathname[0] == '.'){
        printf("Directory cannot be removed\n");
        return -1;
    }
    int isEmpty = 0; 
    //parse path
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);
    if(pi->DEPointer->dirType != 1){
        printf("Is not a directory\n");
        return -1;
    }
    
    if(pi->DEPointer->size != DE_STRUCT_SIZE*2){
        isEmpty = 1;
    }
    
    if(isEmpty == 1){
        printf("Directory is not empty!\n");
        return -1;
    }
    
    char* parentPath = getParentDirectory(pi->path);

    
    pathInfo* parentPi = malloc(sizeof(pathInfo));
    parentPi->DEPointer = malloc(sizeof(dirEntry));

    parentPi = parsePath(parentPath);
    
    dirEntry* tempEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
    loadDirEntries(tempEntries, parentPi->DEPointer->location);

    
    tempEntries[pi->value].name[0] = '\0';
    tempEntries[pi->value].dirType = -1;
    releaseFreeSpace(vcb.freeSpaceBitMap, tempEntries[pi->value].location, tempEntries[pi->value].size);
    tempEntries[pi->value].location = -1;
    tempEntries[pi->value].size = 0;
    tempEntries[pi->value].extentLocation = -1;

    
    updateBitMap(vcb.freeSpaceBitMap);
    tempEntries[0].size -= DE_STRUCT_SIZE;
    
    if(tempEntries[0].location == tempEntries[1].location){
        tempEntries[1].size -= DE_STRUCT_SIZE;
    }else{
        
        char* parentDir = getLastPathElement(cwdPath);
        dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
        LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        
        for(int i = 0; i < MAX_DIRENT_SIZE; i++){
            if(strcmp(parentDir, tempDEntries[i].name) == 0){
                
                tempDEntries[i].size -= DE_STRUCT_SIZE;
                
                i = MAX_DIRENT_SIZE;
            }
        }
        //write to disk
        LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        free(tempDEntries);
    }
    
    LBAwrite(tempEntries, DIRECTORY_BLOCKSIZE, tempEntries[0].location);
   
    LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

   
    free(tempEntries);
    free(parentPi->DEPointer);
    free(parentPi);
    free(pi->DEPointer);
    free(pi);
    return 0;   
}

//--------------------fs_delete------------------------

int fs_delete(char* filename){

    //parse path
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(filename);
    if(pi->value < 0){
        printf("File doesn't exist\n");
        return -1;
    }
    if(pi->DEPointer->dirType != 0){
        printf("Is not a file\n");
        return -1;
    }

    
    cwdEntries[pi->value].name[0] = '\0';
    cwdEntries[pi->value].dirType = -1;
    
    cwdEntries[pi->value].location = -1;
    cwdEntries[pi->value].size = 0;
    releaseFile(cwdEntries[pi->value].extentLocation);
    
    releaseFreeSpace(vcb.freeSpaceBitMap, cwdEntries[pi->value].extentLocation, EXTTABLE_BLOCK_SIZE);
    cwdEntries[pi->value].extentLocation = -1;

    updateBitMap(vcb.freeSpaceBitMap);
    cwdEntries[0].size -= DE_STRUCT_SIZE;
    
    if(cwdEntries[0].location == cwdEntries[1].location){
        cwdEntries[1].size -= DE_STRUCT_SIZE;
    }else{
        
        char* parentDir = getLastPathElement(cwdPath);
        dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
		LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        
        for(int i = 0; i < MAX_DIRENT_SIZE; i++){
            
            if(strcmp(parentDir, tempDEntries[i].name) == 0){
                
                tempDEntries[i].size -= DE_STRUCT_SIZE;
                
                i = MAX_DIRENT_SIZE;
            }
        }
        
        LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        free(tempDEntries);

    }
    LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
    
    LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

    free(pi->DEPointer);
    free(pi);
    return 0;

};	


//----------------------------fs_move--------------------
//Helper routine 

int fs_move(char* src, char* dest){
    //parse path 

    pathInfo* srcPi = malloc(sizeof(pathInfo));
    srcPi->DEPointer = malloc(sizeof(dirEntry));
    pathInfo* destPi = malloc(sizeof(pathInfo));
    destPi->DEPointer = malloc(sizeof(dirEntry));
    
    srcPi = parsePath(src); //parsed info 
    destPi = parsePath(dest);   //parsed info 


    //validate
    if(srcPi->value < 0){
        printf("Source File doesn't exist\n");
        return -1;
    }

    if(destPi->value == -2){
        printf("Destination directory doesn't exist\n");
        return -1;
    }
    
    
    
    int isDir = fs_isDir(destPi->path); 
    

    char* oldCwdPath = malloc(strlen(cwdPath)+1);
    strcpy(oldCwdPath, cwdPath);
    
    if(isDir != 1){
        char* parentDirDest = getParentDirectory(destPi->path);
        fs_setcwd(parentDirDest);
    }else{
        fs_setcwd(destPi->path);
    }

    int fileIndex;
    
    if(destPi->value == -1 || isDir == 1){
        
        for(int i = 0; i < MAX_DIRENT_SIZE; i++){
            if(cwdEntries[i].dirType == -1){
                fileIndex = i;
                //Exit Loop
                i = MAX_DIRENT_SIZE;
            }
        }
    }else{
        
        fileIndex = destPi->value;
    }
    
   
    char* destName;
    
    if(isDir != 1)
        destName = getLastPathElement(destPi->path);
    else
        destName = getLastPathElement(srcPi->path);

    
    strcpy(cwdEntries[fileIndex].name, destName);
    cwdEntries[fileIndex].dirType = 0;
    cwdEntries[fileIndex].location = srcPi->DEPointer->location;
    cwdEntries[fileIndex].created = srcPi->DEPointer->created;
    time(&cwdEntries[fileIndex].lastModified);
    cwdEntries[fileIndex].size = srcPi->DEPointer->size;
    cwdEntries[fileIndex].extentLocation = srcPi->DEPointer->extentLocation;
    
    cwdEntries[0].size +=  DE_STRUCT_SIZE;

    

    
    if(cwdEntries[0].location == cwdEntries[1].location){
        cwdEntries[1].size += DE_STRUCT_SIZE;
    }else{
        
        
        char* parentDir = getLastPathElement(cwdPath);
        dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
		LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        
        for(int i = 0; i < MAX_DIRENT_SIZE; i++){
            if(strcmp(parentDir, tempDEntries[i].name) == 0){
                
                tempDEntries[i].size += DE_STRUCT_SIZE;
                //Exit loop
                i = MAX_DIRENT_SIZE;
            }
        }
        //Write back to disk
        LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        free(tempDEntries);
        
    }

    char* parentDirSrc = getParentDirectory(srcPi->path);
    
    fs_setcwd(parentDirSrc);  
    
    fs_delete(srcPi->path);
    
    fs_setcwd(oldCwdPath);
    
    //Free all the allocated memories
    free(oldCwdPath);
    free(srcPi->DEPointer);
    free(srcPi);
    free(destPi->DEPointer);
    free(destPi);

    return 0;

}
