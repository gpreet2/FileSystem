/**************************************************************
 Class:  CSC-415-01 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
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
#include "freeSpace.h"
#include "vcbInt.h"
#include "dirStructInt.h"


#define MAGIC_NUMBER 0x5A3C7F2A5A3C7F2A

//Redeclaring the vcb global variable
VCB vcb;

//Helper Function
/**
 * Takes a pointer to bitMap(freespace) and size of a block
 * initializes bitMap in the volume
*/
void initBitMap(char* bitMapPointer, u_int64_t blockSize){
    unsigned char tempByte = 0xFC;
	//1111 1100 Represent the first 6 blocks used by VCB and the Bitmap itself
	//Setting the first byte at index 0
    bitMapPointer[0] = 0xFC;
	//Initializing all the other bytes as free space
    for(int i = 1; i < 5*blockSize; i++){
        bitMapPointer[i] = 0x00;
    }
	//Write back into disk
    LBAwrite(bitMapPointer, 5, 1);
}


int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */

	char* vcbBlock = malloc(blockSize);

	//Read the first block (VCB) to vcbBlock buffer
	LBAread(vcbBlock, 1, 0);
	//Copying VCB struct variables from vcbBlock into the VCB struct
	memcpy(&vcb, vcbBlock, sizeof(VCB));
	
	if(vcb.signature != MAGIC_NUMBER){
		//Initialize VCB variables
		printf("INITIALIZING VOLUME CONTROL BLOCK\n");
		vcb.signature = MAGIC_NUMBER;
		vcb.numBlocks = numberOfBlocks;
		vcb.blockSize = blockSize;

		//Initialize Freespace
		//Should come out to be 5 if numberOfBlocks = 19531 & blockSize = 512
		int bitMapBlockSize = ((numberOfBlocks + 7)/8 + (blockSize -1))/blockSize;
		//Setting up the freespace pointer array
		vcb.freeSpaceBitMap = malloc(bitMapBlockSize*blockSize);
		//Keeping track of the size of the bitmap array
		vcb.bitMapByteSize = bitMapBlockSize*blockSize;
		initBitMap(vcb.freeSpaceBitMap, blockSize);
		vcb.freeSpace = 1;
		
		//Initialize RootDirectory
		//Our Directory Entry is 64 bytes long
		int numOfDirEntries = MAX_DIRENT_SIZE; //6 blocks 
		int sizeOfDirectory = 6; 
		dirEntry* rootDir = malloc(numOfDirEntries*sizeof(dirEntry));
		
		//Setting the directory entries to their free state
		for(int i = 0; i < numOfDirEntries; i++){
			rootDir[i].name[0] = '\0';
			rootDir[i].dirType = -1; //free state
			rootDir[i].size = 0;
			rootDir[i].location = -1;
			rootDir[i].extentLocation = -1;
		}
		int freeBlockIndex = getConsecFreeSpace(vcb.freeSpaceBitMap, vcb.bitMapByteSize, 6);
		

		//Set up the "." Directory Entry
		strcpy(rootDir[0].name, ".");
		rootDir[0].size = (int) 2*sizeof(dirEntry);
		// 1 for Directory type directory Entry
		rootDir[0].dirType = 1;
		rootDir[0].location = freeBlockIndex;
		//Setting the time created and last modified to the current time
		time(&rootDir[0].created);
		time(&rootDir[0].lastModified);

		//Set up the ".." Directory Entry, repeat the step
		strcpy(rootDir[1].name, "..");
		rootDir[1].size = (int) 2*sizeof(dirEntry);
		rootDir[1].dirType = 1;
		rootDir[1].location = freeBlockIndex;
		time(&rootDir[1].created);
		time(&rootDir[1].lastModified);

		//Writing the root Directory into disk
		LBAwrite(rootDir, sizeOfDirectory, freeBlockIndex);
		//Writing the VCB back into disk
		vcb.RootDir = freeBlockIndex;

		//Update bitmap on disk
		updateBitMap(vcb.freeSpaceBitMap);
		//Write VCB back onto disk
		memcpy(vcbBlock, &vcb, sizeof(VCB));
		LBAwrite(vcbBlock, 1, 0);

	
		free(rootDir);

	}else{
		//Load the Bitmap into the VCB Bitmap Pointer
		int bitMapBlockSize = 5*blockSize;
		vcb.freeSpaceBitMap = malloc(5*blockSize);
		LBAread(vcb.freeSpaceBitMap, 5, vcb.freeSpace);
	}

	free(vcbBlock);
	
	//Initializing cwdPath and cwdEntries
	initGlobalVar();
	

	return 0;
	}
	
//exit file system	
void exitFileSystem ()
	{
		free(vcb.freeSpaceBitMap); //free the memory
		freeGlobalVar();
		printf ("System exiting\n");
	}
