/**************************************************************

* Class:  CSC-415

* Name: Professor Bierman

* Student ID: N/A

* Project: Basic File System

* File: fsInit.c

* Description: Main driver for the file system assignment.

**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fsLow.h"
#include "mfs.h"

// Define constants for volume size and block size
#define VOLUME_SIZE 10000000
#define BLOCK_SIZE 512

// Global VCB variable
struct fs_vcb vcb;

// Function to display the VCB contents
void displayVCB(struct fs_vcb* vcb) {
    printf("Volume Size: %lu\n", vcb->volumeSize);
    printf("Block Size: %lu\n", vcb->blockSize);
    // Add more fields as needed
}

// Function to exit the file system
void exitFileSystem() {
    printf("System exiting\n");
}

// Initialize the free space management data structure
void initFreeSpace(uint64_t totalBlocks) {
    uint64_t* freeSpace = (uint64_t*)malloc(totalBlocks * sizeof(uint64_t));
    if (freeSpace == NULL) {
        exitFileSystem();
        exit(1);
    }
    for (uint64_t i = 0; i < totalBlocks; i++) {
        freeSpace[i] = 1; // Set all blocks as free
    }
    // Add error handling for b_write
    if (b_write(1, (char*)freeSpace, totalBlocks * sizeof(uint64_t)) != 0) {
        free(freeSpace);
        exitFileSystem();
        exit(1);
    }
    free(freeSpace);
}

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    if (numberOfBlocks != VOLUME_SIZE || blockSize != BLOCK_SIZE) {
        return -1; // Return an error code if they don't match
    }

    // Initialize the Volume Control Block (VCB)
    vcb.volumeSize = numberOfBlocks;
    vcb.blockSize = blockSize;

    // Initialize the Free Space Management
    initFreeSpace(numberOfBlocks);

    // Create the root directory
    struct fs_diriteminfo rootDirectory[2];
    struct fs_diriteminfo currentDirEntry, parentDirEntry;
    strcpy(currentDirEntry.d_name, ".");
    currentDirEntry.fileType = FT_DIRECTORY;
    strcpy(parentDirEntry.d_name, "..");
    parentDirEntry.fileType = FT_DIRECTORY;
    rootDirectory[0] = currentDirEntry;
    rootDirectory[1] = parentDirEntry;

    // Add debugging output
    printf("Initializing the root directory...\n");

    // Write the VCB, free space data, and root directory to the filesystem
    if (b_write(0, (char*)&vcb, sizeof(struct fs_vcb) != 0)) {
        exitFileSystem();
        return -1;
    }
    if (b_write(2, (char*)rootDirectory, 2 * sizeof(struct fs_diriteminfo) != 0)) {
        exitFileSystem();
        return -1;
    }

    // Display the VCB contents
    displayVCB(&vcb);

    return 0; // Indicate success
}

