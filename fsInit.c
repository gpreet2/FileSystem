/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
* File: fsInit.c
*
* Description: The fsInit.c file is crucial for setting up the initial state of
* the file system, ensuring the proper allocation of blocks, and initializing the
* necessary structures and metadata to manage the file system's functionality.
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fsLow.h"
#include "mfs.h"
#include "mapping.h"
#include "vcb.h"
#include "dirInit.h"


#define MAGIC_NUMBER 0xEFB112C2EFB112C2

//Re-declaration of the global variable VCB
VCB vcb;

//Helper Function
//Initializes the bitMap in the volume based on block size.
void initBitMap(char* bitMapPointer, u_int64_t blockSize) {
    unsigned char tempByte = 0xFC;
	//Represents the initial 6 blocks used by VCB alongside the Bitmap itself

    bitMapPointer[0] = 0xFC;
	//Initializing all other bytes as free space
    for (int i = 1; i < 5*blockSize; i++) {
        bitMapPointer[i] = 0x00;
    }

	//Writing back into disk
    LBAwrite(bitMapPointer, 5, 1);
}


int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize) {
	printf ("Initializing File System with %ld blocks with a block size of %ld\n",
			 numberOfBlocks, blockSize);

	char* vcbBlock = malloc(blockSize);

	//Read the first block (VCB) to vcbBlock buffer
	LBAread(vcbBlock, 1, 0);

	//Copying VCB struct variables from vcbBlock into the VCB struct
	memcpy(&vcb, vcbBlock, sizeof(VCB));
	
	if (vcb.signature != MAGIC_NUMBER) {
		//Initialize VCB variables
		printf("Initializing Volume Control Block (VCB)\n");
		vcb.signature = MAGIC_NUMBER;
		vcb.numBlocks = numberOfBlocks;
		vcb.blockSize = blockSize;

		//Initialize Freespace
		int bitMapBlockSize = ((numberOfBlocks + 7)/8 + (blockSize -1))/blockSize;
		vcb.freeSpaceBitMap = malloc(bitMapBlockSize*blockSize);
		vcb.bitMapByteSize = bitMapBlockSize*blockSize;
		initBitMap(vcb.freeSpaceBitMap, blockSize);
		vcb.freeSpace = 1;
		
		//Initialize RootDirectory
		int numOfDirEntries = MAX_DIRENT_SIZE; 		//six blocks 
		int sizeOfDirectory = 6; 
		dirEntry* rootDir = malloc(numOfDirEntries*sizeof(dirEntry));
		
		//Setting the directory entries to their free state
		for (int i = 0; i < numOfDirEntries; i++) {
			rootDir[i].name[0] = '\0';
			rootDir[i].dirType = -1; 		//Free state
			rootDir[i].size = 0;
			rootDir[i].location = -1;
			rootDir[i].extentLocation = -1;
		}
		int freeBlockIndex = getConsecFreeSpace(vcb.freeSpaceBitMap, vcb.bitMapByteSize, 6);
		

		//Set up the "." Directory Entry
		strcpy(rootDir[0].name, ".");
		rootDir[0].size = (int) 2 * sizeof(dirEntry);
		rootDir[0].dirType = 1;
		rootDir[0].location = freeBlockIndex;
		time(&rootDir[0].created);
		time(&rootDir[0].lastModified);

		//Set up the ".." Directory Entry and replicate the process.
		strcpy(rootDir[1].name, "..");
		rootDir[1].size = (int) 2 * sizeof(dirEntry);
		rootDir[1].dirType = 1;
		rootDir[1].location = freeBlockIndex;
		time(&rootDir[1].created);
		time(&rootDir[1].lastModified);

		//Writing the root Directory into disk
		LBAwrite(rootDir, sizeOfDirectory, freeBlockIndex);
		vcb.RootDir = freeBlockIndex;

		//Update bitmap on disk
		updateBitMap(vcb.freeSpaceBitMap);

		//Write VCB back onto disk
		memcpy(vcbBlock, &vcb, sizeof(VCB));
		LBAwrite(vcbBlock, 1, 0);
	
		free(rootDir);

	} else {
		//Initialize VCB's free space bitmap if the signature exists
		int bitMapBlockSize = 5 * blockSize;
		vcb.freeSpaceBitMap = malloc(5 * blockSize);

		//Read the free space bitmap from disk into the VCB's free space bitmap
		LBAread(vcb.freeSpaceBitMap, 5, vcb.freeSpace);
	}
	
	//Free the memory allocated for the VCB block
	free(vcbBlock);
	
	//Initialize global variables for current working directory path and entriess
	initGlobalVar();
	
	//Return 0 indicating successful initialization
	return 0;
	}
	
//Function to exit the file system	
void exitFileSystem () {
	//Free the memory allocated for the VCB's free space bitmap
		free(vcb.freeSpaceBitMap);
		//Free global variables 
		freeGlobalVar();
		printf ("System Exiting\n");
	}
