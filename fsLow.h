#ifndef _FSLOW_H
#define _FSLOW_H
#define VOLUME_SIZE 10000000
#define BLOCK_SIZE 512


#include <stdint.h>

// Volume Control Block (VCB) structure
struct fs_vcb {
    uint64_t volumeSize;    // Total size of the volume
    uint64_t blockSize;     // Size of each block
    uint64_t total_blocks;  // Total number of blocks
    uint64_t root_directory; // Block number of the root directory
    // Add other fields as required by your project
};

// Other filesystem-related structures and function declarations can be included here

// Function to start the partition system
int startPartitionSystem(char *filename, uint64_t *volSize, uint64_t *blockSize);

// Function to close the partition system
int closePartitionSystem(void);

// Function for low-level file write (LBAwrite)
uint64_t LBAwrite(void *buffer, uint64_t lbaCount, uint64_t lbaPosition);

// Function for low-level file read (LBAread)
uint64_t LBAread(void *buffer, uint64_t lbaCount, uint64_t lbaPosition);

// Other function declarations related to your low-level filesystem operations

#endif

