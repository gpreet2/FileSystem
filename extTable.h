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
* Description: The extTable.h file provides a set of functions and structures
* essential for managing file extents within the file system, facilitating
* allocation, tracking, and deallocation of file blocks on the disk.
*
**************************************************************/

#ifndef EXTTABLE_H
#define EXTTABLE_H

#define NUMBER_OF_EXTTABLE 64	
#define EXTTABLE_BLOCK_SIZE 1	//Extent table size is 1 block (512 bytes)

typedef struct extent {
	int location;	 	//Starting location of the extent
	int count;	    	//Number of blocks in the extent
	
} extent;

//Helper routines that helps implementation of extent table
//Retrieve extent table at given location
extent* getExtentTable(int extentLocation);

//Initialize an extent table at a given location	
void initExtentTable(int extentLocation);

//Add entry to extent table
int addToExtentTable(extent* extentTable, int location, int count);

//Get Logical Block Address from extent table
int getLBAFromFile(extent* extentTable, int location);

//Release all blocks associated with the file
void releaseFile(int extentLocation);

//Release blocks no longer part of the file
void releaseFreeBlocksExtent(extent* extentTable, int location);

//Update the extent table
void updateExtentTable(extent* extentTable, int extentLocation);

//Display content of the extent table
void printExtentTable(extent* extentTable);

#endif
