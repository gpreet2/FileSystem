/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
* File: extTable.c
*
* Description: the extent table.c file  encapsulates the functionality required to
* manage the extent table, ensuring efficient allocation, merging of contiguous
* blocks, and proper handling of file block releases within the basic file system
* project.
*
**************************************************************/

#include "extTable.h"
#include <stdio.h>
#include <stdlib.h>
#include "fsLow.h"
#include "mapping.h"
#include "vcb.h"

//Helper Function
//Function to determine the size of the extent table
int getExtentTableSize(extent* extentTable){
    int size = 0;
    for (int i = 0; i < NUMBER_OF_EXTTABLE; i++) {
        if (extentTable[i].location != -1) {  //-1 indicates free state
            size++;
        } else {
            //Exit loop when encountering free state
            i = NUMBER_OF_EXTTABLE;
        }
    }
    return size;
}   

//Helper function to add the element in the extent Table
//Merges consecutive rows within the extent table
void mergeNewRow(extent* extentTable) {
    int maxRow = getExtentTableSize(extentTable);
    for (int i = 0; i < maxRow - 1; i++) {
        //Merging contiguous rows 
        if(extentTable[i].location + extentTable[i].count == extentTable[maxRow -1].location) {
            //Merge the two contiguous rows 
            extentTable[i].count += extentTable[maxRow -1].count;
            extentTable[maxRow -1].location = -1;
            extentTable[maxRow -1].count = -1;
        }
    }

}

//Obtains a pointer to the extent table based on its location in the free space bitmap
extent* getExtentTable(int extentLocation) {
    extent* extentTable = malloc(NUMBER_OF_EXTTABLE*sizeof(extent));
    LBAread(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    return extentTable;
}

//Initializes the extent table at a given location
void initExtentTable(int extentLocation) {
    extent* extentTable = malloc(NUMBER_OF_EXTTABLE*sizeof(extent));
    LBAread(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);

    //Setting all entries to free state (-1)
    for (int i = 0; i < NUMBER_OF_EXTTABLE; i++) {
        extentTable[i].location = -1;
        extentTable[i].count = -1;
    }

    //Writing the initialized extent table to the location
    LBAwrite(extentTable, EXTTABLE_BLOCK_SIZE, extentLocation);
    free(extentTable);
}

//Helper routine to add values in extent table
int addToExtentTable(extent* extentTable, int location, int count) {
    int flag = 0;
    //Iterating through the extent table
    for (int i = 0; i < NUMBER_OF_EXTTABLE; i++) {
        //Found the free extent
        if (extentTable[i].location == -1) {  
            //Add to the extent table
            extentTable[i].location = location;
            extentTable[i].count = count;
            i = NUMBER_OF_EXTTABLE;      //Exit loop after adding
            flag = 1;
        }
    }

    //checking if there is enough space in extent table
    if (flag == 0) {
        printf("Exceeded row limit in the extent table\n");
        return -1;  // Unable to add to the extent table
    };

    //Merge contiguous rows if possible
    mergeNewRow(extentTable);
    return 0;
}

int getLBAFromFile(extent* extentTable, int location) {
    //Local variables to keep track
    int i = 0;
    int result;
    int index = location;  
    //If index at 0 its the start location of the file
    if (index == 0) {
        return extentTable[0].location;     // Start location of the file
    }
    

    while ( index > 0) {
        //Move through our extent table and subtracting count
        if (index > extentTable[i].count) {
            //Case: index is greater than count means it's not the i location
            index = index - extentTable[i].count;
            i++;
        } else if (index == extentTable[i].count) {
            //Case: index is equal meaning it's the next location
            i++;
            index = 0;
            result = extentTable[i].location;
        } else {
            //Case: the file location is between the i location and its count
            result = extentTable[i].location + index;
            index = 0;
        }
    }
    return result;      //Returns the location from the extent table
}

//helper routine to set the extent table of the file to free state
void releaseFile(int extentLocation){
    extent* extentTable = getExtentTable(extentLocation);
    for (int i = 0; i < NUMBER_OF_EXTTABLE; i++) {
        if (extentTable[i].location != -1) {
            releaseFreeSpace(vcb.freeSpaceBitMap, extentTable[i].location, extentTable[i].count);
            //printf("Extent, File: %d, %d\n",extentLocation, extentTable[i].location);
            extentTable[i].location = -1;
        } else {
            i = NUMBER_OF_EXTTABLE;     //Exit loop when encountering free state
        }
    }
    updateBitMap(vcb.freeSpaceBitMap);
}

//This function release blocks that are no longer part of the files
//This function will also update the vcb.bitMap
void releaseFreeBlocksExtent(extent* extentTable, int location) {
    //Setting up variables to keep track
    int maxRow = getExtentTableSize(extentTable);
    int lbaPosition = getLBAFromFile(extentTable, location);
    int found = 0;
    //Iterating through the rows in the extent table
    for (int i = 0; i < maxRow; i++) {
        //Found is true when we find which row the block we want to free is in
        if (found == 0) {
            int maxCount = extentTable[i].location + extentTable[i].count;
            //Iterating through each count in a location
            for (int j = extentTable[i].location;j < maxCount; j++) {
                //Found is set to true
                if (lbaPosition == j) {
                    found = 1;
                    extentTable[i].count = j - extentTable[i].location;
                    //Resetting the extent table
                    if (extentTable[i].count == 0) {
                        //Remove the row from the extent table
                        extentTable[i].count = -1;
                        extentTable[i].location = -1;
                    }

                    //Update the free space bitmap
                    releaseFreeSpace(vcb.freeSpaceBitMap, j, 1);
                } else if (found == 1) {
                    releaseFreeSpace(vcb.freeSpaceBitMap, j, 1);
                }
            }
        } else {
            //Removing the row from the extent table
            releaseFreeSpace(vcb.freeSpaceBitMap, extentTable[i].location, extentTable[i].count);
            extentTable[i].count = -1;
            extentTable[i].location = -1;
        }
    }
}

//update extent table -- helper routine
void updateExtentTable(extent* extentTable, int extentLocation) {
    //printf("Overwrite %d\n", extentLocation);
    LBAwrite(extentTable, 1, extentLocation);
}

//printing the content of extent table
void printExtentTable(extent* extentTable) {
    int maxRow = getExtentTableSize(extentTable);
    for (int i = 0; i < maxRow; i++) {
        printf("Row: (%d, %d)\n",extentTable[i].location, extentTable[i].count);
    }
}
