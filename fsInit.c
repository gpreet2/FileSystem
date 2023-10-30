/**************************************************************
* Class:  CSC-415-02 Fall 2023
* Names: Gurpreet Natt
* Student IDs:922883894
* GitHub Name:gpreet2
* Group Name: Group 5
* Project: Basic File System

* File: fsInit.c

* Description: Main driver for the file system assignment.

**************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "fsLow.h"
#include "mfs.h"

// Constants for volume properties and file system identification
#define MAX_VOLUME_NAME_LENGTH 31
#define NULL_TERMINATOR 1
#define FILE_SYSTEM_SIGNATURE 0x1A2B3C4D

// Volume Control Block structure stores metadata about the file system
typedef struct {
    uint32_t signature; // Unique identifier for our file system
    char volumeName[MAX_VOLUME_NAME_LENGTH + NULL_TERMINATOR];
    uint64_t totalBlocks;
    uint64_t blockSize;
    uint64_t totalSize;
    uint64_t freeBlocks;
    uint64_t firstFreeBlock;
    uint64_t freeSpaceBitmapBlock; // Address to the Free Space Bitmap in our file system
    uint64_t rootDirectoryBlock; // Address to the Root Directory
    time_t lastMountedTime;
} VolumeControlBlock;

// Forward declarations for helper functions
void initFreeSpace(VolumeControlBlock* vcb);
void initRootDirectory(VolumeControlBlock* vcb);

// Function to initialize the file system
int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
    
    // Allocating space in memory for the Volume Control Block
    VolumeControlBlock* vcb = (VolumeControlBlock*)malloc(sizeof(VolumeControlBlock));
    if (!vcb) {
        perror("Error allocating memory for VCB");
        return -1;
    }

    // Attempting to read the existing Volume Control Block from the disk
    if (LBAread(vcb, 1, 0) != 1) {
        fprintf(stderr, "Error: Unable to read VCB from block 0\n");
        free(vcb);
        return -1;
    }

    // Checking if the file system has been previously initialized
    if (vcb->signature == FILE_SYSTEM_SIGNATURE) {
        // The signature matches, the volume has been previously initialized
        printf("Volume has already been initialized.\n");
    } else {
        // The signature does not match, indicating a need for initialization
        printf("Volume not initialized. Initializing now...\n");

        // Setting up the initial state and properties of our file system
        vcb->signature = FILE_SYSTEM_SIGNATURE;
        vcb->totalBlocks = numberOfBlocks;
        vcb->blockSize = blockSize;
        vcb->totalSize = numberOfBlocks * blockSize;

        // Initializing free space and root directory
        initFreeSpace(vcb);
        initRootDirectory(vcb);

        // Save the updated Volume Control Block to the disk
        if (LBAwrite(vcb, 1, 0) != 1) {
			// If saving fails, print an error message and exit
            fprintf(stderr, "Error: Unable to save settings to disk\n");
            free(vcb);
            return -1;
        }
    }

    // Releasing the allocated memory 
	//for the Volume Control Block as it's no longer needed in memory
    free(vcb);
    return 0;
}


void exitFileSystem(){
	printf("Exiting\n");
}