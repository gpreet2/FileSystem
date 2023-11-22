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
* Description: Interface for bitMap
*
**************************************************************/

#ifndef MAPPING_H
#define MAPPING_H

#define MAPPING_SIZE 5   //size of bitMap in VCB is 5 blocks
#define MAPPING_LOCATION 1 // location in VCB is at index 1



int getConsecFreeSpace(unsigned char* bitMap, int bitMapSize, int numOfBlocks); //get contiguous free space in disk
int releaseFreeSpace(unsigned char* bitMap, int location, int size);    //freeing the space in disk
void updateBitMap(unsigned char* bitMap);   //write back to disk.

#endif
