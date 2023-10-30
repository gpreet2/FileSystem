/**************************************************************
 Class:  CSC-415-01 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: freeSpace.h
*
* Description: Interface for bitMap
*
**************************************************************/
#ifndef FREESPACE_H
#define FREESPACE_H

#define BITMAP_SIZE 5   //size of bitMap in VCB is 5 blocks
#define BITMAP_LOCATION 1 // location in VCB is at index 1
// int bitCounter(unsigned char myByte);
// unsigned char mask(int offset);
// int freeSpaceCounter(unsigned char myByte);
// int checkABit(unsigned char myByte, int offset);
// int checkForConsecFreeSpace(unsigned char myByte, int count);

// void setABit(unsigned char* bitMap, int offset);
// void clearABit(unsigned char* bitMap, int offset);


int getConsecFreeSpace(unsigned char* bitMap, int bitMapSize, int numOfBlocks); //get contiguous free space in disk
int releaseFreeSpace(unsigned char* bitMap, int location, int size);    //freeing the space in disk
void updateBitMap(unsigned char* bitMap);   //write back to disk.

#endif
