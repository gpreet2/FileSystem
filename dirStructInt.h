/**************************************************************
 Class:  CSC-415-01 Fall 2023
* Names: Babak Milani , Mozhgan Ahsant, Bisum Tiwana, Gurpreet Natt
* Student IDs: 920122577, 921771510, 920388011, 922883894
* GitHub Name: babakmilani, AhsantMozhgan, SpindlyGold019, gpreet2
* Group Name: Team 05
* Project: Basic File System
*
* File: dirStructInt.h
*
* Description: Directory Interface.
*
**************************************************************/
#ifndef DIRSTRUCTINT_H
#define DIRSTRUCTINT_H

#include <time.h>

#define DIRECTORY_BLOCKSIZE 6
#define MAX_DIRENT_SIZE 48
#define DE_STRUCT_SIZE 64

//Directory Entry struct with the size of 64 bytes
typedef struct dirEntry{
	char name[32];
	time_t created;
	time_t lastModified;
	int location;
	int size;
	//Free state is -1 || is a file directory entry 0 || is a directory 1
	int dirType;
	int extentLocation;
	
} dirEntry;

#endif
