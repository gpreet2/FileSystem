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

//Directory Entry 
typedef struct dirEntry{
	char name[32];
	time_t created;
	time_t lastModified;
	int location;
	int size;
	
	int dirType;
	int extentLocation;
	
} dirEntry;

#endif
