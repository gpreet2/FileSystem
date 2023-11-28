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
* Description: freespace volume
*
**************************************************************/

#include "mapping.h"

#include <stdio.h>
#include <stdlib.h>
#include "fsLow.h"


//  1 = used
//  0 = free
int bitCounter(unsigned char myByte){
    int x = 0;
    if((myByte & 0x80) == 0x80) x++;
    if((myByte & 0x40) == 0x40) x++;
    if((myByte & 0x20) == 0x20) x++;
    if((myByte & 0x10) == 0x10) x++;
    if((myByte & 0x08) == 0x08) x++;
    if((myByte & 0x04) == 0x04) x++;
    if((myByte & 0x02) == 0x02) x++;
    if((myByte & 0x01) == 0x01) x++;
    return x;
} 

unsigned char mask(int offset){
    switch(offset){
        case 0:
            return 0x80;
        case 1:
            return 0x40;
        case 2:
            return 0x20;
        case 3:
            return 0x10;
        case 4:
            return 0x08;
        case 5:
            return 0x04;
        case 6:
            return 0x02;
        case 7: 
            return 0x01;
        default:
            printf("Offset must be between 0-7\n");
            return 0x00;
    }
}

//  1 = used
//  0 = free
int freeSpaceCounter(unsigned char myByte){
    return 8 - bitCounter(myByte); // gives you the remaining number of free blocks in a byte
};


void setABit(unsigned char *bitMap, int offset){
    
    int byteIndex = offset/8;
    unsigned char tempByte = mask((offset % 8));
    bitMap[byteIndex] = (bitMap[byteIndex] | tempByte);
}



void clearABit(unsigned char *bitMap, int offset){
    int byteIndex = offset/8;   // 1 byte = 8 bits
    unsigned char tempByte = mask((offset % 8));
    tempByte = ~tempByte;
    bitMap[byteIndex] = (bitMap[byteIndex] & tempByte);
}

int checkABit(unsigned char myByte, int offset){
    unsigned char tempByte = mask(offset);
    if((myByte & tempByte) == 0x00){
        return 0;
    }else{
        return 1;
    }
}


/**
 * Helper routine 

*/
int getConsecFreeSpace(unsigned char* bitMap, int bitMapSize, int numOfBlocks){
    int firstFreeBlock;
    int firstFreeByte;
    int minFreeBytesNeeded = (numOfBlocks + 7)/8;
    int freeConsecBytes = 0;

    for(int i =0; i < bitMapSize; i++){
        if(bitMap[i] == 0x00){
            if(freeConsecBytes == 0){
                firstFreeBlock = i*8;
                firstFreeByte = i; 
            }
            freeConsecBytes++;

        }else{
            freeConsecBytes = 0;
        }

        if(freeConsecBytes == minFreeBytesNeeded){
            
            for(int j = 7; j >= 0; j--){
                if(checkABit(bitMap[firstFreeByte-1], j) == 0){
                    firstFreeBlock = (firstFreeByte-1)*8 + j;
                }else{
                    
                    j = -1;
                }
            }
            //break the for loop
            i = bitMapSize;
        }
    }

    //Fail to find free blocks
    if(freeConsecBytes == 0){
        return -1;
    }

    for(int i = 0; i < numOfBlocks; i++){
        setABit(bitMap, firstFreeBlock + i);
    }

    return firstFreeBlock;
}
//helper routine 

int releaseFreeSpace(unsigned char* bitMap, int location, int size){
    
    for(int i = location; i < location+size; i++){
        clearABit(bitMap, i);
        
    }
    return 0;
}

void updateBitMap(unsigned char* bitMap){
	LBAwrite(bitMap, MAPPING_SIZE, MAPPING_LOCATION);
}
