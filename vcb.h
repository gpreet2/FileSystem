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
* Description: VCB Interface
**************************************************************/

#ifndef VCB_H
#define _VCB_H
//Size of VCB is 24 bytes
//Volume Control Block struct

typedef struct VCB{
	//unique magic number
	long signature;

	int numBlocks;
	int blockSize;
	int freeSpace;
	//Memory Pointer allocated at runtime
	unsigned char* freeSpaceBitMap;
	int bitMapByteSize;
	int RootDir;
} VCB;

extern VCB vcb;

#endif
