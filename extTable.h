/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: extTable.h
*
* Description: extent table interface
*
**************************************************************/

#ifndef EXTTABLE_H
#define EXTTABLE_H

#define NUMBER_OF_EXTTABLE 64	
#define EXTTABLE_BLOCK_SIZE 1	//size of extent table is 1 block --> 512 bytes

typedef struct extent {
	int location;
	int count;
	
} extent;

//helper routines that helps implementation of extent table

extent* getExtentTable(int extentLocation);	
void initExtentTable(int extentLocation);
int addToExtentTable(extent* extentTable, int location, int count);
int getLBAFromFile(extent* extentTable, int location);
void releaseFile(int extentLocation);
void releaseFreeBlocksExtent(extent* extentTable, int location);
void updateExtentTable(extent* extentTable, int extentLocation);
void printExtentTable(extent* extentTable);

#endif
