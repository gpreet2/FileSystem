/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


#include "b_io.h"
#include "mapping.h"
#include "mfs.h"
#include "vcb.h"
#include "fsLow.h"
#include "extTable.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define INIT_FILE_SIZE 10	//when creating a file
#define ADDITIONAL_FILE_BLOCK 50	//when a file needs more space

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	//int buflen;		//holds how many valid bytes are in the buffer
	//ADDED
	int fileOffset;   //holds the index tracking the whole file
	int fileSize;	//holds the actual file size
	int fileBlocks;	//holds how many blocks in total we allocate to the file
	int flag; 		//holds the permission of the file
	int directoryLocation;	//holds the parent directory location
	int positionInDE;	//holds the position of the file inside its parent directory
	extent* extentTable; //holds the extent table of the file
	int extentLocation;	//holds the location of the extent table of the file

	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	

b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's

	if (returnFd < 0)
	{
		return -1;
	}
	//parsing the path
	pathInfo* pi = malloc(sizeof(pathInfo));
	pi->DEPointer = malloc(sizeof(dirEntry));
	pi = parsePath(filename);

	//validating the path
	if(pi->value == -2){
		printf("Path is invalid\n");
		return -1;
	}
	
	//if flags is O_CREAT and path is correct but file doesn't exist
	if(flags & O_CREAT == O_CREAT && pi->value == -1){

		//Find free directory Entry inside parent directory
		int index = -1;
		for(int i = 0; i < MAX_DIRENT_SIZE; i++){
			if(cwdEntries[i].dirType == -1 && index == -1){
				index = i;
			}
		}
		//checking if directory is full
		if(index == -1){
			printf("Directory is full\n");
			return -1;
		}
		//Initialize File Directory Entry inside Parent Directory
		strcpy(cwdEntries[index].name, filename);
		cwdEntries[index].dirType = 0;

		//Finding the free space
		cwdEntries[index].extentLocation = 
			getConsecFreeSpace(vcb.freeSpaceBitMap, vcb.bitMapByteSize, EXTTABLE_BLOCK_SIZE);
		//Initializing the extent table with our free space
		initExtentTable(cwdEntries[index].extentLocation);
		int fileFreeSpace = getConsecFreeSpace(vcb.freeSpaceBitMap, vcb.bitMapByteSize, INIT_FILE_SIZE);
		
		if(fileFreeSpace == -1 || cwdEntries[index].extentLocation == -1){
			printf("No more free space\n");
			return -1;
		}
		//update the current working Directory
		cwdEntries[index].location = fileFreeSpace;
		cwdEntries[index].size = 0;
		time(&cwdEntries[index].created);
		time(&cwdEntries[index].lastModified);

		
		extent* extentTable = getExtentTable(cwdEntries[index].extentLocation);
		//Adding the first extent
		addToExtentTable(extentTable, cwdEntries[index].location, INIT_FILE_SIZE);
		updateExtentTable(extentTable, cwdEntries[index].extentLocation);

		cwdEntries[0].size += DE_STRUCT_SIZE;
		//Update .. if its the root directory
		if(cwdEntries[0].location == cwdEntries[1].location){
			cwdEntries[1].size += DE_STRUCT_SIZE;
		}else{
			//Update Parent Directory on the size
			dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
			LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
			for(int i = 0; i < MAX_DIRENT_SIZE; i++){
				if(strcmp(getLastPathElement(cwdPath), tempDEntries[i].name) == 0){
					tempDEntries[i].size += DE_STRUCT_SIZE;
					//Exit loop
					i = MAX_DIRENT_SIZE;
				}
			}
			LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE,  cwdEntries[1].location);
			free(tempDEntries);
		}
	
		//Write to disk
		LBAwrite(extentTable, EXTTABLE_BLOCK_SIZE,cwdEntries[index].extentLocation);
		updateBitMap(vcb.freeSpaceBitMap);
		LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
		//Reload cwd
		LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
	
		// //Set up FCB
		fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
		fcbArray[returnFd].buf[0] ='\0';
		fcbArray[returnFd].index = 0;
		fcbArray[returnFd].fileOffset = 0;
		fcbArray[returnFd].fileSize = 0;
		fcbArray[returnFd].fileBlocks = INIT_FILE_SIZE;
		fcbArray[returnFd].flag = flags;
		fcbArray[returnFd].directoryLocation = cwdEntries[0].location;
		fcbArray[returnFd].positionInDE = index;
		fcbArray[returnFd].extentTable = extentTable;
		fcbArray[returnFd].extentLocation = cwdEntries[index].extentLocation;
		
		return returnFd;

	}
	
	//path exists and file also exists.
	if(pi->value >= 0){
		//initializing the variables in fcb struct array
		fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
		fcbArray[returnFd].buf[0] ='\0';
		fcbArray[returnFd].index = 0;
		fcbArray[returnFd].fileOffset = 0;
		fcbArray[returnFd].fileSize = pi->DEPointer->size;
		fcbArray[returnFd].positionInDE = pi->value;
		fcbArray[returnFd].fileBlocks = (fcbArray[returnFd].fileSize + B_CHUNK_SIZE -1)/B_CHUNK_SIZE;

		fcbArray[returnFd].directoryLocation = cwdEntries[0].location;

		fcbArray[returnFd].flag = flags;
		fcbArray[returnFd].extentTable = getExtentTable(pi->DEPointer->extentLocation);
		fcbArray[returnFd].extentLocation = pi->DEPointer->extentLocation;
		
		//checking if the flag is O_TRUNC
		if((flags & O_TRUNC) == O_TRUNC){
			if((flags & O_WRONLY) == O_WRONLY || (flags & O_RDWR) == O_RDWR){
				//empty the file
				cwdEntries[pi->value].size = 0;
				fcbArray[returnFd].fileSize = 0;
				
			}else{
				printf("No write permission to truncate file\n");
				return -1;
			}	
		}
		//update the current working Directory
		time(&cwdEntries[pi->value].lastModified);
		LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);	
		//Reload cwd
		LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

	}else{
		printf("Error opening file! File does not exist!\n");
		return -1;
	}
	
	//free the memory used 
	free(pi->DEPointer);
	free(pi);
	
	return (returnFd);						// all set
	
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	if(whence & SEEK_SET == SEEK_SET){
		fcbArray[fd].fileOffset = offset; //setting the file offset to the given offset
	}else if((whence & SEEK_CUR) == SEEK_CUR){
		fcbArray[fd].fileOffset += offset;	//The file offset is set to its current location plus offset
	}else if(whence & SEEK_END == SEEK_END){
		//The file offset is set to the size of the file plus offset bytes.
		fcbArray[fd].fileOffset += fcbArray[fd].fileOffset + offset;
	}else{
		printf("Invalid SEEK flags\n");
	}
		
	return fcbArray[fd].fileOffset; 
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	//local variables to help tracking
	int neededBytes = count;
	int callerBufferOffset = 0;

	//Check if there is a write permission
	if(!(((fcbArray[fd].flag & O_WRONLY) == O_WRONLY) || 
		((fcbArray[fd].flag & O_RDWR) == O_RDWR))){
		printf("No permission to write\n");
		return -1; 
	}

	//remaining bytes in the file
	int remainingBytes = fcbArray[fd].fileBlocks*B_CHUNK_SIZE - fcbArray[fd].fileSize;
	//Check if we need more blocks
	if(count > remainingBytes){
		//get more space using helper routing getConsecFreeSpace
		int newFileLocation = getConsecFreeSpace(vcb.freeSpaceBitMap, vcb.bitMapByteSize, ADDITIONAL_FILE_BLOCK);
		//check if there is enough space in disk
		if(newFileLocation == -1){
			printf("Disk is full\n");
			return -1;
		}
		//update the free space map
		updateBitMap(vcb.freeSpaceBitMap);
		//update the file size
		fcbArray[fd].fileBlocks += ADDITIONAL_FILE_BLOCK;	
		//update extent table
		int result = addToExtentTable(fcbArray[fd].extentTable, newFileLocation, ADDITIONAL_FILE_BLOCK);	
		//check if extent table is full
		if(result == -1){
			printf("Out of Extent\n");
			return -1;
		}
	}
	//start writing
	while(neededBytes > 0){
		//Reculating the number of bytes we have left in our buffer
		remainingBytes = B_CHUNK_SIZE - fcbArray[fd].index;
		
		//Check to see if we have left over space in the buffer to write to
		if(remainingBytes > 0){
			//Get the current block position of the buffer
			int currentBlock = fcbArray[fd].fileOffset/B_CHUNK_SIZE; 
			int lbaPosition = getLBAFromFile(fcbArray[fd].extentTable, currentBlock);
			//Setting how much we need to write
			int copyAmount = neededBytes;
			if(neededBytes >= remainingBytes){
				copyAmount = remainingBytes;
			}
			//Load the buffer block in, write to it, and write it back to disk
			LBAread(fcbArray[fd].buf, 1, lbaPosition);
			memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer + callerBufferOffset, copyAmount);
			LBAwrite(fcbArray[fd].buf, 1, lbaPosition);
			//Update variables
			fcbArray[fd].fileOffset += copyAmount;
			fcbArray[fd].index = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
			fcbArray[fd].fileSize += copyAmount;
			neededBytes -= copyAmount;
			callerBufferOffset += copyAmount;
			
			
		}else{
			//We're in a new block starting from position 0
			int currentBlock = fcbArray[fd].fileOffset/B_CHUNK_SIZE;
			int lbaPosition = getLBAFromFile(fcbArray[fd].extentTable, currentBlock);
			//Setting how much we need to write
			int copyAmount = 0;
			if(neededBytes >= B_CHUNK_SIZE){
				copyAmount = B_CHUNK_SIZE;
			}else{
				copyAmount = neededBytes;
			}
			//write to the buffer block, and write it back to disk
			memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer + callerBufferOffset, copyAmount);
			LBAwrite(fcbArray[fd].buf, 1, lbaPosition);
			//Update variables
			fcbArray[fd].fileOffset += copyAmount;
			fcbArray[fd].index = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
			fcbArray[fd].fileSize += copyAmount;
			neededBytes -= copyAmount;
			callerBufferOffset += copyAmount;
		}
	}

	//Load the directory entries from disk
	dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
	LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, fcbArray[fd].directoryLocation);
	//Update the size based on how much we write
	tempDEntries[fcbArray[fd].positionInDE].size += callerBufferOffset;
	//Update the extent table
	updateExtentTable(fcbArray[fd].extentTable,
		tempDEntries[fcbArray[fd].positionInDE].extentLocation);
	//Write back to disk
	LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, fcbArray[fd].directoryLocation);
	//Reload the CWD from disk
	LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
	
	//freeing the memory; safety first
	free(tempDEntries);
	tempDEntries = NULL;
	
	
	return callerBufferOffset; 
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	//Keeping track of the offset in the caller buffer
	int callerBufferOffset = 0;
	//Keeping track of how much we want to memcpy
	int copyAmount = 0;
	//Keeping track of the remaining size of the file
	int remainingFileSize = fcbArray[fd].fileSize - fcbArray[fd].fileOffset;
	//Used to check return value of LBAread
	int returnValue = 0;
	//Keeping track of how many bytes the caller buffer still needs
	int neededBytes = count;
	//Keeping track of which block we are in
	int fileBlockIndex = fcbArray[fd].fileOffset / B_CHUNK_SIZE;
	//Rechecking the buffer index
	fcbArray[fd].index = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
	
	//Setting neededBytes to remainingFileSize instead if neededBytes is larger
	if(neededBytes > remainingFileSize){
		neededBytes = remainingFileSize;
	}

	while(neededBytes > 0){
		//Recalculating local variables
		remainingFileSize = fcbArray[fd].fileSize - fcbArray[fd].fileOffset;

		//Will do LBAread into the caller's buffer directly
		if(fcbArray[fd].index == 0 && (neededBytes) >= B_CHUNK_SIZE){
			// Number of blocks to be copied
			int startingLocation;
			int blockNeeded = neededBytes/B_CHUNK_SIZE;
			copyAmount = blockNeeded*B_CHUNK_SIZE;
		
			//LBAread the file block directly into the caller's buffer
			while(blockNeeded > 0){
				startingLocation = getLBAFromFile(fcbArray[fd].extentTable, fileBlockIndex);
				
				returnValue = LBAread(buffer + callerBufferOffset, 1, startingLocation);
				//Erro checking
				if(returnValue < 0){
					printf("Error LBAread in b_read\n");
					return -1;
				}
				//Updating the FCB and local variables
				fileBlockIndex++;
				fcbArray[fd].fileOffset += B_CHUNK_SIZE;
				callerBufferOffset += B_CHUNK_SIZE;
			}		
		}else{
			//Check if there are any data left in the FCB Buffer
			if(fcbArray[fd].index == 0){
				//LBAread into fcb buffer
				int startingLocation = getLBAFromFile(fcbArray[fd].extentTable, fileBlockIndex);
				returnValue = LBAread(fcbArray[fd].buf, 1, startingLocation);
				//Error checking
				if(returnValue < 0){
					return -1;
				}
			}
			//Check if the neededBytes is larger than the remaining data in the 
			//fcb buffer
			if(neededBytes > B_CHUNK_SIZE - fcbArray[fd].index){
				//Setting copyAmount (For memcpy)
				copyAmount = B_CHUNK_SIZE - fcbArray[fd].index;
				fileBlockIndex++;
			}else{
				//Setting copyAmount (For memcpy)
				copyAmount = neededBytes;
			}
			//Copy data into the caller's buffer with copyAmount set earlier
			memcpy(
			buffer + callerBufferOffset, 
			fcbArray[fd].buf + fcbArray[fd].index, copyAmount);
			//Updating variables
			fcbArray[fd].fileOffset += copyAmount;
			callerBufferOffset += copyAmount;
			fcbArray[fd].index = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
		}
		
		neededBytes -= copyAmount;
	}
	
	
	return callerBufferOffset;
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{	
		//Check if we allocated more blocks for the file then we needed
		if(fcbArray[fd].fileBlocks > (fcbArray[fd].fileSize + B_CHUNK_SIZE -1)/B_CHUNK_SIZE){
			int location = fcbArray[fd].fileSize/B_CHUNK_SIZE;
			location++;
			//Release the free blocks that is not needed
			releaseFreeBlocksExtent(fcbArray[fd].extentTable, location);
			updateExtentTable(fcbArray[fd].extentTable, fcbArray[fd].extentLocation);
			updateBitMap(vcb.freeSpaceBitMap);
			//printExtentTable(fcbArray[fd].extentTable);
			
		}
		//reload cwd
		LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
		//Free all the buffer
		free(fcbArray[fd].buf);
		fcbArray[fd].buf = NULL;
		free(fcbArray[fd].extentTable);
		return 0;
	}
