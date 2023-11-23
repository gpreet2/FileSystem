/**************************************************************
 Class:  CSC-415-02 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: dirInit.h
*
* Description: Directory Interface.
*
**************************************************************/

#ifndef DIRINIT_H
#define DIRINIT_H

#include <time.h>

#define DIRECTORY_BLOCKSIZE 6
#define MAX_DIRENT_SIZE 48
#define DE_STRUCT_SIZE 64

//Defining a Directory Entry struct with a size of 64 bytes
typedef struct dirEntry {
	char name[32];
	time_t created;
	time_t lastModified;
	int location;
	int size;

	//Definition://Free state: -1 indicates free space
	//File directory entry: 0 denotes a file
	//Directory: 1 indicates a directory
	int dirType;
	int extentLocation;
} dirEntry;

#endif
