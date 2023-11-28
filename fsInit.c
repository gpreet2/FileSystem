/**************************************************************
 Class:  CSC-415-02 Fall 2023
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
#include "mapping.h"
#include "vcb.h"
#include "dirInit.h"


#define MAGIC_NUMBER 0xEFB112C2EFB112C2


VCB vcb;

//Helper Function

void initBitMap(char* bitMapPointer, u_int64_t blockSize){
    unsigned char tempByte = 0xFC;
	
    bitMapPointer[0] = 0xFC;
	
    for(int i = 1; i < 5*blockSize; i++){
        bitMapPointer[i] = 0x00;
    }
	
    LBAwrite(bitMapPointer, 5, 1);
}


int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */

	char* vcbBlock = malloc(blockSize);

	
	LBAread(vcbBlock, 1, 0);
	
	memcpy(&vcb, vcbBlock, sizeof(VCB));
	
	if(vcb.signature != MAGIC_NUMBER){
		
		printf("INITIALIZING VOLUME CONTROL BLOCK\n");
		vcb.signature = MAGIC_NUMBER;
		vcb.numBlocks = numberOfBlocks;
		vcb.blockSize = blockSize;

		//Initialize space
		
		int bitMapBlockSize = ((numberOfBlocks + 7)/8 + (blockSize -1))/blockSize;
		//Setting up the space
		vcb.freeSpaceBitMap = malloc(bitMapBlockSize*blockSize);
		//bitmap array
		vcb.bitMapByteSize = bitMapBlockSize*blockSize;
		initBitMap(vcb.freeSpaceBitMap, blockSize);
		vcb.freeSpace = 1;
		
		//Initialize RootDirectory
		
		int numOfDirEntries = MAX_DIRENT_SIZE; //6 blocks 
		int sizeOfDirectory = 6; 
		dirEntry* rootDir = malloc(numOfDirEntries*sizeof(dirEntry));
		
		
		for(int i = 0; i < numOfDirEntries; i++){
			rootDir[i].name[0] = '\0';
			rootDir[i].dirType = -1; //free state
			rootDir[i].size = 0;
			rootDir[i].location = -1;
			rootDir[i].extentLocation = -1;
		}
		int freeBlockIndex = getConsecFreeSpace(vcb.freeSpaceBitMap, vcb.bitMapByteSize, 6);
		

		// "." Directory Entry
		strcpy(rootDir[0].name, ".");
		rootDir[0].size = (int) 2*sizeof(dirEntry);
		
		rootDir[0].dirType = 1;
		rootDir[0].location = freeBlockIndex;
		
		time(&rootDir[0].created);
		time(&rootDir[0].lastModified);

		// ".." Directory Entry
		strcpy(rootDir[1].name, "..");
		rootDir[1].size = (int) 2*sizeof(dirEntry);
		rootDir[1].dirType = 1;
		rootDir[1].location = freeBlockIndex;
		time(&rootDir[1].created);
		time(&rootDir[1].lastModified);

		
		LBAwrite(rootDir, sizeOfDirectory, freeBlockIndex);
		
		vcb.RootDir = freeBlockIndex;

		
		updateBitMap(vcb.freeSpaceBitMap);
		
		memcpy(vcbBlock, &vcb, sizeof(VCB));
		LBAwrite(vcbBlock, 1, 0);

	
		free(rootDir);

	}else{
		//Load Bitmap 
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
