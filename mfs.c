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

//Initializing extern Global variables from mfs.h

char* cwdPath;            //Current working directory path
dirEntry* cwdEntries;     //Current working directory entries

//Load the directory entries from disk into DEArray at 'location'
void loadDirEntries(dirEntry* DEArray, int location) {
    LBAread(DEArray, DIRECTORY_BLOCKSIZE, location);
}
//Initialize global variables for current working directory
void initGlobalVar() {
    cwdPath = malloc(256);  //Allocate memory for the path
    strcpy(cwdPath, "/");   //Set the initial path as root ("/")

    //Allocate memory for directory entries
    cwdEntries = malloc(MAX_DIRENT_SIZE*DE_STRUCT_SIZE);

    //Read directory block into memory and copy into cwdEntries
    //Temporary buffer to hold directory block data
    char* DEBuffer = malloc(DIRECTORY_BLOCKSIZE*512);
    //Read directory block from disk
    LBAread(DEBuffer, DIRECTORY_BLOCKSIZE, vcb.RootDir);
    //Copy directory data to cwdEntries
    memcpy(cwdEntries, DEBuffer, MAX_DIRENT_SIZE*DE_STRUCT_SIZE);
    free(DEBuffer); //Free temporary buffer memory
}

//Free memory occupied by global variables
void freeGlobalVar() {
    free(cwdEntries);
    free(cwdPath);
}

//Get the last element within a path
char * getLastPathElement(const char *pathname) {

    char *str;
    //Get the pointer to the last slash ("/")
    str = strrchr(pathname, '/');

    if (str != NULL) {
        str++;      //Move past the last slash
    }

    return str;
}

//Get the parent directory from a path, excluding the last element
char * getParentDirectory(const char *pathname) {
    //Handling root directory ("/")
    if (pathname[0] == '/' && strlen(pathname) <= 1) {
        char* path = malloc(sizeof(char)*2);
        strcpy(path, pathname);
        return path;
    }

    char* lastElement = getLastPathElement(pathname);
    
    char tempPath[strlen(pathname) + 1];
    strcpy(tempPath, pathname);

    int len = strlen(lastElement);

    //Cases where the parent directory is the root or a regular directory
    if (strlen(pathname) == len+1) {
        //Parent is root ("/home" => "/")
        tempPath[strlen(pathname) - len] = '\0';
    } else {

        //Regular scenario ("/home/testFolder1" => "/home")
        tempPath[strlen(pathname) - len - 1] = '\0';
    }
   //Allocate memory for the path
    char* path = malloc(strlen(tempPath) + 1);
    memcpy(path, tempPath, strlen(tempPath) + 1);   //Copy the path data
    return path;
}


//Function to parse a pathname and validate its components.
pathInfo* parsePath(const char *pathname) {
    pathInfo* result = malloc(sizeof(pathInfo));
    result->DEPointer = malloc(sizeof(dirEntry));

    //Variables initialization
    int entryIndex = 0;
    char *delim = "/";
    char tempPath[256];
    tempPath[0] = '\0';
    strncpy(tempPath, pathname, strlen(pathname) + 1);
    
    //Memory allocations
    dirEntry* tempDirEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
    char * DEBuffer = malloc(DIRECTORY_BLOCKSIZE*512);

    
    //Handling relative and absolute paths
    if (pathname[0] != '/') {
        strcpy(tempPath, cwdPath);
        strcat(tempPath, "/");
        strncat(tempPath, pathname, strlen(pathname));    
    }
    
    //Loading directory entries
    loadDirEntries(tempDirEntries, vcb.RootDir);  
    
    //tokenize path with / as delimeter.
    char *token = strtok(tempPath, delim);
    char *pathTokens[64];
    pathTokens[0] = NULL;

    int tokenIndex = 0;
    
    //Handling "." and ".." to simplify the path
    while(token != NULL) {
        if(strcmp(token, "..") == 0){
            if(tokenIndex == 0){
                //Handle root directory or invalid cases
            } else {
                //Move to the parent directory
                tokenIndex--;
                pathTokens[tokenIndex] = NULL;
            }
                
        } else if(strcmp(token, ".") == 0) {
            //Do nothing as "." represents the current directory
        } else {
            //Add directory tokens to the stack
            pathTokens[tokenIndex] = token;
            tokenIndex++;
        }
        token = strtok(NULL, delim);
    }

    // Handling the root path or an empty path
    if (tokenIndex == 0) {
        
        char* temp = ".";
        pathTokens[0] = temp;
        tokenIndex++;
        
    }
    pathTokens[tokenIndex] = NULL;
    
    //Create a simplified path
    char* absolutePath = malloc(64);
    absolutePath[0] = '\0';
    for(int i = 0; i < tokenIndex; i++){
        if(strcmp(pathTokens[0], ".") == 0) {
            strcat(absolutePath, delim);
        } else {
            //Construct the simplified path
            strcat(absolutePath, delim);
            strcat(absolutePath, pathTokens[i]);
        }
    }
    
    strcpy(result->path, absolutePath);
    free(absolutePath);
    
    //Checking for directory entry existence in the path
    int exists = 0;
    int tokenCounter = 0;

   
    while (pathTokens[tokenCounter] != NULL) {
        for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
            //loop through the directory entries to look for matching directory entry
            if(tempDirEntries[i].dirType != -1
             && strcmp(tempDirEntries[i].name, pathTokens[tokenCounter]) == 0) {
                exists = 1;
                entryIndex = i;
                i = MAX_DIRENT_SIZE;
            }
        }

        if (exists == 0) {
            if (tokenCounter != tokenIndex - 1) {
                entryIndex = -2;
                result->DEPointer = NULL;
                break;
            } else {
                entryIndex = -1;
                result->DEPointer = NULL;
                break;
            }
        } else if (exists == 1) {
            
            if (tokenCounter != tokenIndex - 1) {
                loadDirEntries(tempDirEntries, tempDirEntries[entryIndex].location);
                exists = 0;
                tokenCounter++;
            } else {
                
                //Found the file/directory and return the DE index of Dir[n-1]
                //Copy the Directory entry into the result Directory entry
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


//Open directory function
fdDir * fs_opendir(const char *pathname) {
    
    //Beginning with path parsing
    //The parse path function yields a Path Info struct.
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);

    //Check if the path is valid, and whether it's actually a directory
    if (pi->value >= 0) {
        if (pi->DEPointer->dirType != 1) {
            printf("Not a directory\n");
            return NULL;
        }
        fdDir* fd = malloc(sizeof(fdDir));
        
        //Setting up the fd struct elements once the directory is open.       
        fd->dirPointer = malloc(MAX_DIRENT_SIZE*DE_STRUCT_SIZE);
        loadDirEntries(fd->dirPointer, pi->DEPointer->location);
        fd->d_reclen = sizeof(fdDir);
        fd->directoryStartLocation = pi->DEPointer->location;
        fd->dirEntryPosition = 0;
        fd->dirSize = (fd->dirPointer[0].size)/DE_STRUCT_SIZE;
        fd->fileIndex = 0;

        free(pi->DEPointer);
        free(pi);
        return fd;
    } else {
        free(pi->DEPointer);
        free(pi);
        return NULL;
    }
}


//----------------------fs_readDir------------------------------

//Reading directory contents function
struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    //Malloc the return struct
    struct fs_diriteminfo* ii = malloc(sizeof(struct fs_diriteminfo));
    int exist = 0;

    if (dirp->fileIndex == dirp->dirSize) {
        return NULL;
    }

    //looping through each element
    for (int i = dirp->dirEntryPosition; i < MAX_DIRENT_SIZE; i++) {
        if (dirp->dirPointer[i].dirType == 0 || dirp->dirPointer[i].dirType == 1) {
            strcpy(ii->d_name, dirp->dirPointer[i].name);
            //Setting up the elements in the return struct
            ii->d_reclen = (int) sizeof(struct fs_diriteminfo);
            if (dirp->dirPointer[i].dirType == 1) {
                ii->fileType = '1';
            } else {
                ii->fileType = '0';
            }
            
            dirp->fileIndex++;
            i = MAX_DIRENT_SIZE;
            exist = 1;
        }
        dirp->dirEntryPosition++;
    }

    if(exist == 0) {
        ii = NULL;
    }

    return ii;
}
//--------------------fs_closedir-------------------
int fs_closedir(fdDir *dirp) {
    //Clean up allocated memory and set the pointer to NULL for safety.
    free(dirp->dirPointer);
    free(dirp);
    dirp = NULL;
}

//----------------fs_getcwd-----------------------------
char *fs_getcwd(char *pathname, size_t size) {
    //Copy the current working directory path into the provided buffer.
    strncpy(pathname,cwdPath,size);//just copy the pathname from cwdPath
    return pathname;
}

//Setting the current working directory
int fs_setcwd(char *pathname) {
    //Write the current working directory back to disk.
    LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

    if (strcmp(pathname, "./") == 0 ) {
        return 0;
    }
    
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);
    
    if (pi->value >= 0) { 
        if (pi->DEPointer->dirType != 1) {
            return -1;
        }
       
        //Set the current working directory entries and path.
        loadDirEntries(cwdEntries, pi->DEPointer->location);
        strcpy(cwdPath,pi->path);
        
        free(pi->DEPointer);
        free(pi);
        return 0;       //Success
    }
    free(pi->DEPointer);
    free(pi);
    return -1;           //Failure
}

//Returns 0 if directory was successfully created at the specified path, -1 otherwise.
int fs_mkdir(const char *pathname, mode_t mode) {
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);

    if (pi->value != -1) {
        return -1;      //Indicates an error
    }

    //Extract the parent directory path.
    char *parentPath = getParentDirectory(pi->path);
    
    //Extract the last element from the path.
    char *lastElementOfPath = getLastPathElement(pi->path);
    
    //Change the current working directory to the parent path.
    if (fs_setcwd(parentPath) == -1) {
        printf("Error: Unable to set parent path as current working directory");
        return -1;
    }

    //Find a free entry in the current working directory.
    int numOfDirEntries = MAX_DIRENT_SIZE;
    int indexOfNewDirEntry;

    for (int i = 0; i < numOfDirEntries; i++) {
        if (cwdEntries[i].dirType == -1) {   
            indexOfNewDirEntry = i;
            //Initialize the directory entry of a new directory.
            strcpy(cwdEntries[i].name, lastElementOfPath);
            //Set directory attributes.
            cwdEntries[i].dirType = 1;
            cwdEntries[i].size = DE_STRUCT_SIZE*2;
            cwdEntries[i].location = getConsecFreeSpace(vcb.freeSpaceBitMap, 
                                    vcb.bitMapByteSize, DIRECTORY_BLOCKSIZE);
            updateBitMap(vcb.freeSpaceBitMap);
            time(&cwdEntries[i].created);
            time(&cwdEntries[i].lastModified);
            cwdEntries[0].size += DE_STRUCT_SIZE;
            
            //Update the parent directory size if it's not the root directory.
            if (cwdEntries[0].location == cwdEntries[1].location) {
                cwdEntries[1].size += DE_STRUCT_SIZE;
            } else {
                //Update the parent directory by loading the parent directory from disk
                char* parentDir = getLastPathElement(cwdPath);
                dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
                LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
                for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
                    if (strcmp(parentDir, tempDEntries[i].name) == 0) {
                        tempDEntries[i].size += DE_STRUCT_SIZE;
                        i = MAX_DIRENT_SIZE;       //Exit loop
                    }
                }

                LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
                free(tempDEntries);
            }
            i = numOfDirEntries;    //Exit loop
        }
    }

    //Further directory initialization
    dirEntry *dirEntries = malloc(MAX_DIRENT_SIZE * DE_STRUCT_SIZE);
    loadDirEntries(dirEntries, cwdEntries[indexOfNewDirEntry].location);
    //Initializing all the directory entries inside our new directory
    for (int i = 0; i < numOfDirEntries; i++) {
        //Initialize directory entries inside the new directory.
        dirEntries[i].name[0] = '\0';
        dirEntries[i].dirType = -1; // free state
        dirEntries[i].size = 0;
        dirEntries[i].location = -1;
        dirEntries[i].extentLocation = -1;
    }

    //Set up "." and ".." directory entries.
    //Write the new directory and current working directory entries to disk.
    LBAwrite(dirEntries, DIRECTORY_BLOCKSIZE, dirEntries[0].location);
    LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

    free(dirEntries);
    free(pi->DEPointer);
    free(pi);

    return 0;       //Success if directory was created.
}

//---------------isFile----------------------
//Return 1 if the given filename corresponds to a file, 0 otherwise.
int fs_isFile(char * filename) {
    //Parse the provided pathname to obtain path information.   
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(filename);

    // Check if the value from parsePath indicates the directory exists (value >= 0).
    if (pi->value >= 0) {   
        //Check if the directory entry type indicates a directory (dirType == 1).
        if (pi->DEPointer->dirType == 0) {   
            free(pi->DEPointer);
            free(pi);      
            return 1;       //Indicates the pathname corresponds to a directory.
        } else {
            free(pi->DEPointer);
            free(pi);
            return 0;   //Indicates the pathname does not correspond to a directory.
        }
    } else {
        free(pi->DEPointer);
        free(pi);
        return 0;      //Indicates the pathname does not correspond to a directory.
    }   
}


//------------- fs_isDir --------------
//Return 1 if the provided pathname corresponds to a directory, 0 otherwise.
int fs_isDir(char * pathname) {   
    //Parse the given pathname to obtain path information.
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);

    //Check if the value from parsePath indicates the existence of the directory (value >= 0).
    if (pi->value >= 0) {   
        //Check if the directory entry type indicates a directory (dirType == 1).
        if (pi->DEPointer->dirType == 1) {       
            free(pi->DEPointer);
            free(pi);    
            return 1;   //Indicates the pathname corresponds to a directory.
        } else {
            free(pi->DEPointer);
            free(pi);
            return 0;   //Indicates the pathname does not correspond to a directory
        }

    } else {
        free(pi->DEPointer);
        free(pi);
        return 0;
    }

}


//----------------fs_stat()----------------
int fs_stat(const char *path, struct fs_stat *buf) {
    //Parse the provided path to obtain directory entry information
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(path);
    
    //Check if the path parsing was successful
    if (pi->value >= 0) {
        //Set values in the provided 'buf' structure with directory entry data
        buf->st_accesstime = pi->DEPointer->lastModified;
        buf->st_size = pi->DEPointer->size;
        buf->st_createtime = pi->DEPointer->created;
        buf->st_modtime = pi->DEPointer->lastModified;

        //Free allocated memory for pathInfo structure
        free(pi->DEPointer);
        free(pi);
        return 1;   //Indicate success
    }

    //Free allocated memory on path parsing failure
    free(pi->DEPointer);
    free(pi);

    return -1;   //Return failure indication
}

//--------------------fs_rmdir---------------------------------
int fs_rmdir(const char *pathname) {
    //Check if the path is the current directory or root
    if (pathname[0] == '.') {
        printf("Unable to remove directory\n");
        return -1;
    }
    int isEmpty = 0; //Flag to verify if the directory is empty

    //Parse the given path
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(pathname);

    //Check if the given path is not a directory
    if (pi->DEPointer->dirType != 1) {
        printf("Not a directory\n");
        return -1;
    }
    
    //Check if the directory is not empty
    if (pi->DEPointer->size != DE_STRUCT_SIZE * 2) {
        isEmpty = 1;
    }

    //Prevent removal of a non-empty directory
    if (isEmpty == 1) {
        printf("The directory is not empty\n");
        return -1;
    }
    
    //Obtain the parent path using a helper routine
    char* parentPath = getParentDirectory(pi->path);

    //Parse the parent path
    pathInfo* parentPi = malloc(sizeof(pathInfo));
    parentPi->DEPointer = malloc(sizeof(dirEntry));
    parentPi = parsePath(parentPath);

    //Create a temporary directory entry to load information about the parent directory
    dirEntry* tempEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
    loadDirEntries(tempEntries, parentPi->DEPointer->location);

    //Set directory entry to free state
    tempEntries[pi->value].name[0] = '\0';
    tempEntries[pi->value].dirType = -1;
    releaseFreeSpace(vcb.freeSpaceBitMap, tempEntries[pi->value].location, tempEntries[pi->value].size);
    tempEntries[pi->value].location = -1;
    tempEntries[pi->value].size = 0;
    tempEntries[pi->value].extentLocation = -1;

    //Update the bitmap
    updateBitMap(vcb.freeSpaceBitMap);
    tempEntries[0].size -= DE_STRUCT_SIZE;
    
    //Update ".." if it's the root directory
    if (tempEntries[0].location == tempEntries[1].location) {
        tempEntries[1].size -= DE_STRUCT_SIZE;
    } else {
        //Update the parent directory 
        char* parentDir = getLastPathElement(cwdPath);
        dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
        LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);

        //Find a matching name in the parent directory
        for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
            if (strcmp(parentDir, tempDEntries[i].name) == 0) {
                //Update the directory entry size
                tempDEntries[i].size -= DE_STRUCT_SIZE;
                i = MAX_DIRENT_SIZE;      //Exit loop  
            }
        }

        //Write changes to disk
        LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        free(tempDEntries);
    }

    //Write changes to disk
    LBAwrite(tempEntries, DIRECTORY_BLOCKSIZE, tempEntries[0].location);
    //Reload directory
    LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

    //Cleanup allocated memory
    free(tempEntries);
    free(parentPi->DEPointer);
    free(parentPi);
    free(pi->DEPointer);
    free(pi);
    return 0;   //Indicate success
}

//--------------------fs_delete------------------------

int fs_delete(char* filename) {

    //Parse the provided filename to obtain directory entry information
    pathInfo* pi = malloc(sizeof(pathInfo));
    pi->DEPointer = malloc(sizeof(dirEntry));
    pi = parsePath(filename);

    //Check if the file exists
    if (pi->value < 0) {
        printf("File doesn't exist\n");
        return -1;
    }
    if (pi->DEPointer->dirType != 0) {
        printf("Is not a file\n");
        return -1;
    }

    //Set directory entry to a free state
    cwdEntries[pi->value].name[0] = '\0';
    cwdEntries[pi->value].dirType = -1;
    cwdEntries[pi->value].location = -1;
    cwdEntries[pi->value].size = 0;
    releaseFile(cwdEntries[pi->value].extentLocation);
    releaseFreeSpace(vcb.freeSpaceBitMap, cwdEntries[pi->value].extentLocation, EXTTABLE_BLOCK_SIZE);
    cwdEntries[pi->value].extentLocation = -1;
    updateBitMap(vcb.freeSpaceBitMap);
    cwdEntries[0].size -= DE_STRUCT_SIZE;

    //Update parent directory size if not root
    if (cwdEntries[0].location != cwdEntries[1].location) {
        char* parentDir = getLastPathElement(cwdPath);
        dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
		LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);

        //Iterate through the parent directory to find matching name
        for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
            if(strcmp(parentDir, tempDEntries[i].name) == 0){
                tempDEntries[i].size -= DE_STRUCT_SIZE;
                i = MAX_DIRENT_SIZE;        //Exit loop
            }
        }

        //Write changes back to disk
        LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        free(tempDEntries);
    }

    //Write changes to disk and reload current directory
    LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
    LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

    //Clean up memory
    free(pi->DEPointer);
    free(pi);
    return 0;
}


//----------------------------fs_move--------------------
int fs_move(char* src, char* dest) {

    //Parse paths for both source and destination
    pathInfo* srcPi = malloc(sizeof(pathInfo));
    srcPi->DEPointer = malloc(sizeof(dirEntry));
    pathInfo* destPi = malloc(sizeof(pathInfo));
    destPi->DEPointer = malloc(sizeof(dirEntry));
    
    srcPi = parsePath(src); //parsed info of source path
    destPi = parsePath(dest);   //parsed info of destination path


    //Validate paths
    if (srcPi->value < 0) {
        printf("The source file does not exist\n");
        return -1;
    }

    if (destPi->value == -2) {
        printf("The destination directory does not exist\n");
        return -1;
    }
    
    
    //Check if the destination is a directory
    int isDir = fs_isDir(destPi->path); 
    
    //Save current working directory path
    char* oldCwdPath = malloc(strlen(cwdPath)+1);
    strcpy(oldCwdPath, cwdPath);
    
    //Change the path to the destination directory or parent directory of destination file
    if (isDir != 1) {
        char* parentDirDest = getParentDirectory(destPi->path);
        fs_setcwd(parentDirDest);
    } else {
        fs_setcwd(destPi->path);
    }

    int fileIndex;
    //Check if the file doesn't exist but the directory exists
    if (destPi->value == -1 || isDir == 1) {
        //Find a free directory entry
        for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
            if (cwdEntries[i].dirType == -1) {
                fileIndex = i;
                //Exit Loop
                i = MAX_DIRENT_SIZE;
            }
        }
    } else {
        //If the file already exists, set it to the value returned by parsePath
        fileIndex = destPi->value;
    }

    char* destName;
    //Set the name of the new file
    if (isDir != 1) {
        destName = getLastPathElement(destPi->path);
    } else {
        destName = getLastPathElement(srcPi->path);
    }

    //Copy the source directory entry to the destination directory entries
    strcpy(cwdEntries[fileIndex].name, destName);
    cwdEntries[fileIndex].dirType = 0;
    cwdEntries[fileIndex].location = srcPi->DEPointer->location;
    cwdEntries[fileIndex].created = srcPi->DEPointer->created;
    time(&cwdEntries[fileIndex].lastModified);
    cwdEntries[fileIndex].size = srcPi->DEPointer->size;
    cwdEntries[fileIndex].extentLocation = srcPi->DEPointer->extentLocation;
    //Update the directory size
    cwdEntries[0].size +=  DE_STRUCT_SIZE;

   //Update the size of the parent directory
    if (cwdEntries[0].location != cwdEntries[1].location) {
        char* parentDir = getLastPathElement(cwdPath);
        dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
		LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);

        //Iterate through the directory to find a matching name
        for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
            if (strcmp(parentDir, tempDEntries[i].name) == 0) {
                //Update the size of the file DE in the parent directory
                tempDEntries[i].size += DE_STRUCT_SIZE;
                //Exit loop
                i = MAX_DIRENT_SIZE;
            }
        }

        //Write changes back to disk
        LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
        free(tempDEntries);
    }

    char* parentDirSrc = getParentDirectory(srcPi->path);
    //Set tthe path to the source file's destination
    fs_setcwd(parentDirSrc);  
    //Delete the source file
    fs_delete(srcPi->path);
    //Set the path back to the original path
    fs_setcwd(oldCwdPath);
    
    //Free all the allocated memories resources
    free(oldCwdPath);
    free(srcPi->DEPointer);
    free(srcPi);
    free(destPi->DEPointer);
    free(destPi);

    return 0;
}
