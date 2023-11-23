/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: mapping.c
*
* Description: The mapping.c file plays a crucial role in managing the allocation
* and deallocation of free space within the file system. These functions help
* identify, mark, and release blocks within the file system's volume based on the
* bitmap representation of free space, ensuring efficient allocation of available
* blocks for files and directories.
*
**************************************************************/

#include "mapping.h"

#include <stdio.h>
#include <stdlib.h>
#include "fsLow.h"

// Count the number of set bits in a byte
int bitCounter(unsigned char myByte) {
    int count = 0;
    //Checking each bit using bitwise AND operation
    for (int i = 0; i < 8; ++i) {
        if ((myByte >> i) & 1) {
            count++;
        }
    }
    return count;
}

//Generate a mask for a specific bit offset
unsigned char mask(int offset) {
    if (offset < 0 || offset > 7) {
        printf("Offset must be between 0-7\n");
        return 0x00;
    }
    return (1 << offset);
}

//Count the number of free blocks in a byte
int freeSpaceCounter(unsigned char myByte) {
    return 8 - bitCounter(myByte); 
}

//Set a bit at a specified offset in a bitmap
void setABit(unsigned char *bitMap, int offset) {
    int byteIndex = offset/8;
    unsigned char tempByte = mask((offset % 8));
    bitMap[byteIndex] = (bitMap[byteIndex] | tempByte);
}

//Clear a bit at a specified offset in a bitmap
void clearABit(unsigned char *bitMap, int offset) {
    int byteIndex = offset/8;   // 1 byte = 8 bits
    unsigned char tempByte = mask((offset % 8));
    tempByte = ~tempByte;
    bitMap[byteIndex] = (bitMap[byteIndex] & tempByte);
}

//Check if a bit at a specified offset is set in a byte
int checkABit(unsigned char myByte, int offset) {
    unsigned char tempByte = mask(offset);
     return ((myByte & tempByte) != 0) ? 1 : 0;
    // if((myByte & tempByte) == 0x00){
    //     return 0;
    // }else{
    //     return 1;
    // }
}

//Find a sequence of consecutive free blocks in a bitmap
int getConsecFreeSpace(unsigned char* bitMap, int bitMapSize, int numOfBlocks){
    int firstFreeBlock;
    int firstFreeByte;
    int minFreeBytesNeeded = (numOfBlocks + 7) / 8;
    int freeConsecBytes = 0;

    for (int i = 0; i < bitMapSize; i++) {
        if (bitMap[i] == 0x00){
            if (freeConsecBytes == 0) {
                firstFreeBlock = i * 8;
                firstFreeByte = i; 
            }
            freeConsecBytes++;
        } else {
            freeConsecBytes = 0;
        }

        if (freeConsecBytes == minFreeBytesNeeded) {
            for (int j = 7; j >= 0; j--) {
                if (checkABit(bitMap[firstFreeByte - 1], j) == 0){
                    firstFreeBlock = (firstFreeByte - 1) * 8 + j;
                } else {
                    j = -1;
                }
            }
            i = bitMapSize;
        }
    }

    //Fail to find a sequence of consecutive free blocks in a bitmap
    if(freeConsecBytes == 0) {
        return -1;
    }

    for(int i = 0; i < numOfBlocks; i++) {
        setABit(bitMap, firstFreeBlock + i);
    }

    return firstFreeBlock;
}

//Release blocks in the bitmap by setting corresponding bits to 0
int releaseFreeSpace(unsigned char* bitMap, int location, int size) {
    for (int i = location; i < location+size; i++) {
        clearABit(bitMap, i);
    }
    return 0;
}
//Write the updated freespace bitmap to the disk
void updateBitMap(unsigned char* bitMap) {
    //Writing the updated bitmap to a specific location on the disk
	LBAwrite(bitMap, MAPPING_SIZE, MAPPING_LOCATION);
}
