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
* Description: The code demonstrates a basic file system implementation that uses
* blocks to organize and manage file data on a disk. It employs an extent table
* structure to track blocks allocated to files, enabling efficient storage and
* retrieval.
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
	//Allocate memory and parse the path
	pathInfo* pi = malloc(sizeof(pathInfo));
	pi->DEPointer = malloc(sizeof(dirEntry));
	pi = parsePath(filename);
	//Validate the path and handle errors
	if (pi->value == -2) {
		printf("Path is not valid\n");
		return -1;
	}
	
	//Check if the file needs to be created and the path is valid
	//but the file doesn't exist
	if (flags & O_CREAT == O_CREAT && pi->value == -1) {
		//Find an empty slot in the parent directory.
		int index = -1;
		for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
			if (cwdEntries[i].dirType == -1 && index == -1) {
				index = i;
			}
		}

		//Check if the directory is full
		if (index == -1) {
			printf("Directory is full\n");
			return -1;
		}

		//Create a new File Directory Entry within the Parent Directory
		strcpy(cwdEntries[index].name, filename);
		cwdEntries[index].dirType = 0;

		//Allocate space and initialize extent table for the file
		cwdEntries[index].extentLocation = getConsecFreeSpace(vcb.freeSpaceBitMap,
										 vcb.bitMapByteSize, EXTTABLE_BLOCK_SIZE);

		initExtentTable(cwdEntries[index].extentLocation);
		int fileFreeSpace = getConsecFreeSpace(vcb.freeSpaceBitMap, 
							vcb.bitMapByteSize, INIT_FILE_SIZE);
		
		//Check for available space
		if (fileFreeSpace == -1 || cwdEntries[index].extentLocation == -1) {
			printf("No more free space\n");
			return -1;
		}

		//Update directory entry information
		cwdEntries[index].location = fileFreeSpace;
		cwdEntries[index].size = 0;
		time(&cwdEntries[index].created);
		time(&cwdEntries[index].lastModified);

		//Manage file extent and update directory sizes
		extent* extentTable = getExtentTable(cwdEntries[index].extentLocation);
		addToExtentTable(extentTable, cwdEntries[index].location, INIT_FILE_SIZE);
		updateExtentTable(extentTable, cwdEntries[index].extentLocation);

		cwdEntries[0].size += DE_STRUCT_SIZE;

		//Update Parent Directory on the size
		if (cwdEntries[0].location == cwdEntries[1].location) {
			dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
			LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, cwdEntries[1].location);
			for (int i = 0; i < MAX_DIRENT_SIZE; i++) {
				if (strcmp(getLastPathElement(cwdPath), tempDEntries[i].name) == 0) {
					tempDEntries[i].size += DE_STRUCT_SIZE;
					i = MAX_DIRENT_SIZE;		//Exit the loop
				}
			}
			LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE,  cwdEntries[1].location);
			free(tempDEntries);
		}
	
		//Write changes to disk and set up File Control Block (FCB)
		LBAwrite(extentTable, EXTTABLE_BLOCK_SIZE,cwdEntries[index].extentLocation);
		updateBitMap(vcb.freeSpaceBitMap);
		LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
		LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
	
		//Establish the File Control Block (FCB)
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
	
	//Check if the path and file exist
	if (pi->value >= 0) {
		//Initialize file control block (FCB) variables
		fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
		fcbArray[returnFd].buf[0] ='\0';
		fcbArray[returnFd].index = 0;
		fcbArray[returnFd].fileOffset = 0;
		fcbArray[returnFd].fileSize = pi->DEPointer->size;
		fcbArray[returnFd].positionInDE = pi->value;
		fcbArray[returnFd].fileBlocks = (fcbArray[returnFd].fileSize + 
											B_CHUNK_SIZE -1)/B_CHUNK_SIZE;
		fcbArray[returnFd].directoryLocation = cwdEntries[0].location;
		fcbArray[returnFd].flag = flags;
		fcbArray[returnFd].extentTable = getExtentTable(pi->DEPointer->extentLocation);
		fcbArray[returnFd].extentLocation = pi->DEPointer->extentLocation;
		
		//Check if the flag is O_TRUNC (truncate file if needed)
		if ((flags & O_TRUNC) == O_TRUNC) {
			if ((flags & O_WRONLY) == O_WRONLY || (flags & O_RDWR) == O_RDWR) {
				cwdEntries[pi->value].size = 0;
				fcbArray[returnFd].fileSize = 0;
			} else {
				printf("Insufficient permissions to truncate the file.\n");
				return -1;
			}	
		}

		//Update directory information and file modification time
		time(&cwdEntries[pi->value].lastModified);
		LBAwrite(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);	
		LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
	} else {
		printf("Failed to open file! File doesn't exist.\n");
		return -1;
	}
	
	//Free allocated memory 
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
	
	//Manage seeking behavior based on 'whence' flags
	if (whence & SEEK_SET == SEEK_SET) {
		//Adjust file offset to the given offset
		fcbArray[fd].fileOffset = offset;
	} else if ((whence & SEEK_CUR) == SEEK_CUR) {
		//Adjust file offset by adding the given offset to its current position
		fcbArray[fd].fileOffset += offset;
	} else if (whence & SEEK_END == SEEK_END) {
		//Move the file offset to the current file size plus the specified offset in bytes
		fcbArray[fd].fileOffset += fcbArray[fd].fileOffset + offset;
	} else {
		printf("Invalid flags used for seeking\n");
	}

	//Return the new file offset	
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
	
	//Local variables for tracking
	int neededBytes = count;
	int callerBufferOffset = 0;

	//Check for write permission
	if (!(((fcbArray[fd].flag & O_WRONLY) == O_WRONLY) || 
		((fcbArray[fd].flag & O_RDWR) == O_RDWR))) {
		printf("Write permission denie\n");
		return -1; 
	}

	//Calculate remaining space in the file
	int remainingBytes = fcbArray[fd].fileBlocks * B_CHUNK_SIZE - fcbArray[fd].fileSize;

	//Check if more blocks are needed for writing
	if (count > remainingBytes) {
		//Get additional space using helper function getConsecFreeSpace
		int newFileLocation = getConsecFreeSpace(vcb.freeSpaceBitMap, 
							vcb.bitMapByteSize, ADDITIONAL_FILE_BLOCK);
		//Verify available disk space
		if (newFileLocation == -1) {
			printf("Disk is full\n");
			return -1;
		}

		//Update free space map and file size information
		updateBitMap(vcb.freeSpaceBitMap);
		fcbArray[fd].fileBlocks += ADDITIONAL_FILE_BLOCK;	

		//Update the extent table and verify if it's at full capacity
		int result = addToExtentTable(fcbArray[fd].extentTable, newFileLocation, 
										ADDITIONAL_FILE_BLOCK);	

		if (result == -1) {
			printf("Exceeded extent limit\n");
			return -1;
		}
	}

	while(neededBytes > 0) {
		//Calculate remaining space in the buffer
		remainingBytes = B_CHUNK_SIZE - fcbArray[fd].index;
		
		// Check if there's space in the current buffer block
		if (remainingBytes > 0) {
			int currentBlock = fcbArray[fd].fileOffset/B_CHUNK_SIZE; 
			int lbaPosition = getLBAFromFile(fcbArray[fd].extentTable, currentBlock);
			
			//Determine the amount to copy, considering available space in the buffer
			int copyAmount = neededBytes >= remainingBytes ? remainingBytes : neededBytes;

			//Load the buffer block in, write to it, and write it back to disk
			LBAread(fcbArray[fd].buf, 1, lbaPosition);
			memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer + callerBufferOffset, copyAmount);
			LBAwrite(fcbArray[fd].buf, 1, lbaPosition);
			
			fcbArray[fd].fileOffset += copyAmount;
			fcbArray[fd].index = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
			fcbArray[fd].fileSize += copyAmount;
			neededBytes -= copyAmount;
			callerBufferOffset += copyAmount;
			
			
		} else {
			//Start at the beginning of a new block (position 0)
			int currentBlock = fcbArray[fd].fileOffset/B_CHUNK_SIZE;
			int lbaPosition = getLBAFromFile(fcbArray[fd].extentTable, currentBlock);

			//Determine the amount to write in a new block
			int copyAmount = neededBytes >= B_CHUNK_SIZE ? B_CHUNK_SIZE : neededBytes;

			//Write to the new buffer block and update variables
			memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer + 
					callerBufferOffset, copyAmount);
			LBAwrite(fcbArray[fd].buf, 1, lbaPosition);

			//Update file variables
			fcbArray[fd].fileOffset += copyAmount;
			fcbArray[fd].index = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
			fcbArray[fd].fileSize += copyAmount;
			neededBytes -= copyAmount;
			callerBufferOffset += copyAmount;
		}
	}

	//Read directory entries from the disk
	dirEntry* tempDEntries = malloc(MAX_DIRENT_SIZE*sizeof(dirEntry));
	LBAread(tempDEntries, DIRECTORY_BLOCKSIZE, fcbArray[fd].directoryLocation);

	//Update size based on write operation
	tempDEntries[fcbArray[fd].positionInDE].size += callerBufferOffset;
	updateExtentTable(fcbArray[fd].extentTable,
					tempDEntries[fcbArray[fd].positionInDE].extentLocation);
	LBAwrite(tempDEntries, DIRECTORY_BLOCKSIZE, fcbArray[fd].directoryLocation);
	LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);
	
	//Free memory
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
	
	//Initialization of variables to track file reading progress
	//Tracks the position in the caller's buffer
	int callerBufferOffset = 0;

	//Tracks the amount to copy
	int copyAmount = 0;

	//Remaining file size
	int remainingFileSize = fcbArray[fd].fileSize - fcbArray[fd].fileOffset;

	//Tracks return value of LBAread
	int returnValue = 0;

	//Tracks bytes needed in the caller buffer
	int neededBytes = count;

	//Tracks the file block index
	int fileBlockIndex = fcbArray[fd].fileOffset / B_CHUNK_SIZE;

	//Recalculating buffer index
	fcbArray[fd].index = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
	
	//Adjust neededBytes to match the remaining file size
	if (neededBytes > remainingFileSize) {
		neededBytes = remainingFileSize;
	}

	//Loop until all required bytes are read
	while (neededBytes > 0) {
		remainingFileSize = fcbArray[fd].fileSize - fcbArray[fd].fileOffset;

		if (fcbArray[fd].index == 0 && (neededBytes) >= B_CHUNK_SIZE) {
			//Read file block directly into the caller's buffer
			int startingLocation;
			int blockNeeded = neededBytes/B_CHUNK_SIZE;
			copyAmount = blockNeeded*B_CHUNK_SIZE;
		
			//LBAread the file block directly into the caller's buffer
			while (blockNeeded > 0) {
				startingLocation = getLBAFromFile(fcbArray[fd].extentTable, fileBlockIndex);
				returnValue = LBAread(buffer + callerBufferOffset, 1, startingLocation);

				if (returnValue < 0) {
					printf("Error encountered in LBA read within function b_read\n");
					return -1;
				}

				//Updating the FCB and local variables
				fileBlockIndex++;
				fcbArray[fd].fileOffset += B_CHUNK_SIZE;
				callerBufferOffset += B_CHUNK_SIZE;
			}		
		} else {
			//Verify if there is any remaining data in the FCB Buffer
			if (fcbArray[fd].index == 0) {
				//Read into FCB buffer
				int startingLocation = getLBAFromFile(fcbArray[fd].extentTable, fileBlockIndex);
				returnValue = LBAread(fcbArray[fd].buf, 1, startingLocation);

				if (returnValue < 0) {
					return -1;
				}
			}

			//Check if the neededBytes exceed the remaining data in the FCB buffer
			if (neededBytes > B_CHUNK_SIZE - fcbArray[fd].index) {
				copyAmount = B_CHUNK_SIZE - fcbArray[fd].index;
				fileBlockIndex++;
			} else {
				copyAmount = neededBytes;
			}

			//Copy data into the caller's buffer
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
	
	//Return the total bytes read
	return callerBufferOffset;
	}
	
//Close the file	
int b_close (b_io_fd fd) {	
		// Check if allocated more blocks than needed
		if (fcbArray[fd].fileBlocks > (fcbArray[fd].fileSize + B_CHUNK_SIZE -1)/B_CHUNK_SIZE) {
			int location = fcbArray[fd].fileSize/B_CHUNK_SIZE;
			location++;

			//Release unnecessary free blocks
			releaseFreeBlocksExtent(fcbArray[fd].extentTable, location);
			updateExtentTable(fcbArray[fd].extentTable, fcbArray[fd].extentLocation);
			updateBitMap(vcb.freeSpaceBitMap);
		}

		//Reload current working directory
		LBAread(cwdEntries, DIRECTORY_BLOCKSIZE, cwdEntries[0].location);

		//Free allocated memory buffers
		free(fcbArray[fd].buf);
		fcbArray[fd].buf = NULL;
		free(fcbArray[fd].extentTable);

		//Return success code
		return 0;
	}
