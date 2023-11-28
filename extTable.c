/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
* File: extTable.c
*
* Description: extent table.
*
**************************************************************/

#include "extTable.h"

#include <stdio.h>
#include <stdlib.h>
#include "fsLow.h"
#include "mapping.h"
#include "vcb.h"

//Helper Function

int getExtentTableSize(extent* extentTable){
    int size = 0;
    for(int i = 0; i < NUMBER_OF_EXTTABLE; i++){
        if(extentTable[i].location != -1){  //-1 means free state
            size++;
        }else{
            //break loop
            i = NUMBER_OF_EXTTABLE;
        }
    }
    return size;
}   


void mergeNewRow(extent* extentTable){
    int maxRow = getExtentTableSize(extentTable);
    //Iterating 
    for(int i = 0; i < maxRow - 1; i++){
        
        if(extentTable[i].location + extentTable[i].count == extentTable[maxRow -1].location){
            
            extentTable[i].count += extentTable[maxRow -1].count;
            extentTable[maxRow -1].location = -1;
            extentTable[maxRow -1].count = -1;
        }
    }

}
//helper routine 

extent* getExtentTable(int extentLocation){
    extent* extentTable = malloc(NUMBER_OF_EXTTABLE*sizeof(extent));
    LBAread(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    return extentTable;
}

//helper routine to initialize extent table


void initExtentTable(int extentLocation){
    extent* extentTable = malloc(NUMBER_OF_EXTTABLE*sizeof(extent));
    LBAread(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    
    for(int i = 0; i < NUMBER_OF_EXTTABLE; i++){
        extentTable[i].location = -1;
        extentTable[i].count = -1;
    }
    
    LBAwrite(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    free(extentTable);
}

//helper routine 
int addToExtentTable(extent* extentTable, int location, int count){
    int flag = 0;
    //Iterating 
    for(int i = 0; i < NUMBER_OF_EXTTABLE; i++){
        
        if(extentTable[i].location == -1){  
            
            extentTable[i].location = location;
            extentTable[i].count = count;
            //exit loop
            i = NUMBER_OF_EXTTABLE;
            flag = 1;
        }
    }

    
    if(flag == 0){
        printf("out of row in the extent table\n");
        return -1;
    };

    mergeNewRow(extentTable);
    return 0;
}

int getLBAFromFile(extent* extentTable, int location){
   
    int i = 0;
    int result;
    int index = location;  
    
    if(index == 0){
        return extentTable[0].location;
    }
    

    while( index > 0){
        
        if(index > extentTable[i].count){
            
            index = index - extentTable[i].count;
            i++;
        }else if(index == extentTable[i].count){
            
            i++;
            index = 0;
            result = extentTable[i].location;
        }else{
            
            result = extentTable[i].location + index;
            index = 0;
        }
    }
    return result;
}

//helper routine 
void releaseFile(int extentLocation){
    extent* extentTable = getExtentTable(extentLocation);
    for(int i = 0; i < NUMBER_OF_EXTTABLE; i++){
        if(extentTable[i].location != -1){
            releaseFreeSpace(vcb.freeSpaceBitMap, extentTable[i].location, extentTable[i].count);
            
            extentTable[i].location = -1;
        }else{
            //exit loop
            i = NUMBER_OF_EXTTABLE;
        }
    }
    updateBitMap(vcb.freeSpaceBitMap);
}



void releaseFreeBlocksExtent(extent* extentTable, int location){
    
    int maxRow = getExtentTableSize(extentTable);
    int lbaPosition = getLBAFromFile(extentTable, location);
    int found = 0;
    //iterating 
    for(int i = 0; i < maxRow; i++){
        //found is true when we find which row the block we want to free is in
        if(found == 0){
            int maxCount = extentTable[i].location + extentTable[i].count;
            //iterating 
            for(int j = extentTable[i].location;j < maxCount; j++){
                
                if(lbaPosition == j){
                    found = 1;
                    extentTable[i].count = j - extentTable[i].location;
                   
                    if(extentTable[i].count == 0){
                        
                        extentTable[i].count = -1;
                        extentTable[i].location = -1;
                    }
                    
                    releaseFreeSpace(vcb.freeSpaceBitMap, j, 1);
                }else if(found == 1){
                    releaseFreeSpace(vcb.freeSpaceBitMap, j, 1);
                }
            }
        }else{
            
            releaseFreeSpace(vcb.freeSpaceBitMap, extentTable[i].location, extentTable[i].count);
            extentTable[i].count = -1;
            extentTable[i].location = -1;
        }
    }
}


void updateExtentTable(extent* extentTable, int extentLocation){
    
    LBAwrite(extentTable, 1, extentLocation);
}


void printExtentTable(extent* extentTable){
    int maxRow = getExtentTableSize(extentTable);
    for(int i = 0; i < maxRow; i++){
        printf("Row: (%d, %d)\n",extentTable[i].location, extentTable[i].count);
    }
}


