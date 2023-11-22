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
//returns the size of the extent table
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

//helper function to add the element in the extent Table
//merge checks if other rows are continuous with the new row
void mergeNewRow(extent* extentTable){
    int maxRow = getExtentTableSize(extentTable);
    //Iterating through the extent table
    for(int i = 0; i < maxRow - 1; i++){
        //The 2 of the rows are continous 
        if(extentTable[i].location + extentTable[i].count == extentTable[maxRow -1].location){
            //Merge the two rows 
            extentTable[i].count += extentTable[maxRow -1].count;
            extentTable[maxRow -1].location = -1;
            extentTable[maxRow -1].count = -1;
        }
    }

}
//helper routine to get pointer to extent table
//takes the location of extent in free space bitMap
//returns a pointer to extent table
extent* getExtentTable(int extentLocation){
    extent* extentTable = malloc(NUMBER_OF_EXTTABLE*sizeof(extent));
    LBAread(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    return extentTable;
}

//helper routine to initialize extent table
//it takes the location of extent location in bitMap

void initExtentTable(int extentLocation){
    extent* extentTable = malloc(NUMBER_OF_EXTTABLE*sizeof(extent));
    LBAread(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    //initializing all in free state
    // free state --> -1
    for(int i = 0; i < NUMBER_OF_EXTTABLE; i++){
        extentTable[i].location = -1;
        extentTable[i].count = -1;
    }
    //Write our new extent table to the location
    LBAwrite(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    free(extentTable);
}

//helper routine to add values in extent table
int addToExtentTable(extent* extentTable, int location, int count){
    int flag = 0;
    //Iterating through the extent table
    for(int i = 0; i < NUMBER_OF_EXTTABLE; i++){
        //found the free extent
        if(extentTable[i].location == -1){  
            //Add to the extent table
            extentTable[i].location = location;
            extentTable[i].count = count;
            //exit loop
            i = NUMBER_OF_EXTTABLE;
            flag = 1;
        }
    }

    //checking if there is enough space in extent table
    if(flag == 0){
        printf("out of row in the extent table\n");
        return -1;
    };

    mergeNewRow(extentTable);
    return 0;
}

int getLBAFromFile(extent* extentTable, int location){
    //Local variables to keep track
    int i = 0;
    int result;
    int index = location;  
    //If index at 0 its the start location of the file
    if(index == 0){
        return extentTable[0].location;
    }
    

    while( index > 0){
        //Move through our extent table and subtracting count
        if(index > extentTable[i].count){
            //Case: index is greater than count means it's not the i location
            index = index - extentTable[i].count;
            i++;
        }else if(index == extentTable[i].count){
            //Case: index is equal meaning it's the next location
            i++;
            index = 0;
            result = extentTable[i].location;
        }else{
            //Case: the file location is between the i location and its count
            result = extentTable[i].location + index;
            index = 0;
        }
    }
    return result;
}

//helper routine to set the extent table of the file to free state
void releaseFile(int extentLocation){
    extent* extentTable = getExtentTable(extentLocation);
    for(int i = 0; i < NUMBER_OF_EXTTABLE; i++){
        if(extentTable[i].location != -1){
            releaseFreeSpace(vcb.freeSpaceBitMap, extentTable[i].location, extentTable[i].count);
            //printf("Extent, File: %d, %d\n",extentLocation, extentTable[i].location);
            extentTable[i].location = -1;
        }else{
            //exit loop
            i = NUMBER_OF_EXTTABLE;
        }
    }
    updateBitMap(vcb.freeSpaceBitMap);
}

//This function release blocks that are no longer part of the files
//This function will also update the vcb.bitMap
void releaseFreeBlocksExtent(extent* extentTable, int location){
    //Setting up variables to keep track
    int maxRow = getExtentTableSize(extentTable);
    int lbaPosition = getLBAFromFile(extentTable, location);
    int found = 0;
    //iterating through the rows in the extent table
    for(int i = 0; i < maxRow; i++){
        //found is true when we find which row the block we want to free is in
        if(found == 0){
            int maxCount = extentTable[i].location + extentTable[i].count;
            //iterating through each count in a location
            for(int j = extentTable[i].location;j < maxCount; j++){
                //Found is set to true
                if(lbaPosition == j){
                    found = 1;
                    extentTable[i].count = j - extentTable[i].location;
                    //Resetting the extent table
                    if(extentTable[i].count == 0){
                        //remove the row from the extent table
                        extentTable[i].count = -1;
                        extentTable[i].location = -1;
                    }
                    //Update the free space bitmap
                    releaseFreeSpace(vcb.freeSpaceBitMap, j, 1);
                }else if(found == 1){
                    releaseFreeSpace(vcb.freeSpaceBitMap, j, 1);
                }
            }
        }else{
            //Removing the row from the extent table
            releaseFreeSpace(vcb.freeSpaceBitMap, extentTable[i].location, extentTable[i].count);
            extentTable[i].count = -1;
            extentTable[i].location = -1;
        }
    }
}

//update extent table -- helper routine
void updateExtentTable(extent* extentTable, int extentLocation){
    //printf("Overwrite %d\n", extentLocation);
    LBAwrite(extentTable, 1, extentLocation);
}

//printing the content of extent table
void printExtentTable(extent* extentTable){
    int maxRow = getExtentTableSize(extentTable);
    for(int i = 0; i < maxRow; i++){
        printf("Row: (%d, %d)\n",extentTable[i].location, extentTable[i].count);
    }
}


// int main(){
//     extent* ext = malloc(64*sizeof(extent));
//     for(int i = 0; i < NUMBER_OF_EXTENT; i++){
//         ext[i].location = -1;
//         ext[i].count = -1;
//     }
//     addToExtentTable(ext, 10, 4);
//     addToExtentTable(ext, 20, 4);
//     addToExtentTable(ext, 104, 2);
//     //addToExtentTable(ext, 106, 8);


//     printExtentTable(ext);
//     //releaseFreeBlocksExtent(ext, 15);
//     printf("\n\n");
//     printExtentTable(ext);
//     return 0;
// }
