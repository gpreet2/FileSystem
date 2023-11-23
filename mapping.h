/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: mapping.h
*
* Description: The mapping.h header file acts as an interface for abstracting the
* implementation details of managing disk space through a bitmap. It allows for a
* standardized approach to handle disk block allocation and release within the
* context of a basic file system project.
*
**************************************************************/

#ifndef MAPPING_H
#define MAPPING_H

#define MAPPING_SIZE 5   // Represents the size of the bitMap in VCB, occupying 5 blocks
#define MAPPING_LOCATION 1 // Represents the index location in VCB where the bitMap is stored


//Retrieve a contiguous free space in the disk bitmap
int getConsecFreeSpace(unsigned char* bitMap, int bitMapSize, int numOfBlocks);

//Free up space in the disk by updating the bitmap
int releaseFreeSpace(unsigned char* bitMap, int location, int size);

//Write the updated bitmap back to the disk
void updateBitMap(unsigned char* bitMap);

#endif
