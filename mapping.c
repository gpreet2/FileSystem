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
* Description: Implementation of functions related to freespace volume
*
**************************************************************/

#include "mapping.h"

#include <stdio.h>
#include <stdlib.h>
#include "fsLow.h"

// Function to count used space in freespace bitmap
int countUsedSpace(unsigned char myByte) {
    int count = 0;
    for (int i = 7; i >= 0; i--) {
        if ((myByte & (1 << i)) != 0) {
            count++;
        }
    }
    return count;
}

// Function to create a mask based on the offset
unsigned char createMask(int offset) {
    return (1 << (7 - offset));
}

// Function to count free blocks in bitmap
int countFreeSpace(unsigned char myByte) {
    return 8 - countUsedSpace(myByte);
}

// Function to set a bit at a given offset in the bitmap
void setBit(unsigned char *bitMap, int offset) {
    int byteIndex = offset / 8;
    int bitOffset = offset % 8;
    bitMap[byteIndex] |= (1 << (7 - bitOffset));
}

// Function to clear a bit at a given offset in the bitmap
void clearBit(unsigned char *bitMap, int offset) {
    int byteIndex = offset / 8;
    int bitOffset = offset % 8;
    bitMap[byteIndex] &= ~(1 << (7 - bitOffset));
}

// Function to check the state of a bit at a given offset in the bitmap
int checkBit(unsigned char myByte, int offset) {
    return ((myByte & createMask(offset)) != 0) ? 1 : 0;
}

// Function to get consecutive free space in the bitmap
int getConsecutiveFreeSpace(unsigned char* bitMap, int bitMapSize, int numOfBlocks) {
    int firstFreeBlock = -1;
    int freeConsecutiveBytes = 0;

    for (int i = 0; i < bitMapSize; i++) {
        if (bitMap[i] == 0x00) {
            if (freeConsecutiveBytes == 0) {
                firstFreeBlock = i * 8;
            }
            freeConsecutiveBytes++;
        } else {
            freeConsecutiveBytes = 0;
        }

        if (freeConsecutiveBytes == numOfBlocks) {
            return firstFreeBlock;
        }
    }

    return -1;
}

// Function to release a block in the bitmap
int releaseFreeSpace(unsigned char* bitMap, int location, int size) {
    for (int i = location; i < location + size; i++) {
        clearBit(bitMap, i);
    }
    return 0;
}

// Function to update the bitmap on the disk
void updateBitmap(unsigned char* bitMap) {
    LBAwrite(bitMap, MAPPING_SIZE, MAPPING_LOCATION);
}

