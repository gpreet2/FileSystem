/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: vcb.h
*
* Description: Volume Control Block (VCB) Interface. This interface establishes
* the structure and essential attributes needed to manage the volume's metadata,
* track available space, and maintain the organization of the file system.
*
**************************************************************/

#ifndef VCB_H
#define _VCB_H
//Size of VCB is 24 bytes
//Volume Control Block struct

typedef struct VCB {
	//Unique magic number identifying the VCB
	long signature;
	//Total number of blocks in the volume
	int numBlocks;
	//Size of each block in bytes
	int blockSize;
	//Number of free blocks in the volume
	int freeSpace;
	//Memory pointer for the free space bitmap
	unsigned char* freeSpaceBitMap;
	//Size of the bitmap in bytes
	int bitMapByteSize;
	//Index of the root directory block
	int RootDir;
} VCB;

extern VCB vcb;		// External declaration of VCB variable

#endif
