// synchdisk.cc 
//	Routines to synchronously access the disk.  The physical disk 
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because 
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone (int arg)
{
    SynchDisk* disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char* name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int) this);
	readerLock = new Lock("reader lock");
	for(int i = 0; i < NumSectors; i++){
		numReader[i] = 0;
		numVis[i] = 0;
		mutex[i] = new Semaphore("sectorC",1);
		
	}
	cache = new Cache[4];
	for(int i = 0; i < 4; i++){
		cache[i].valid = FALSE;
		cache[i].dirty = FALSE;
		cache[i].lru = 0;
		cache[i].sector = -1;
		
	}
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
/*	
	int pos = -1;
	int i;
	for(int i = 0; i < 4;i++){
		if(cache[i].valid == TRUE && cache[i].sector == sectorNumber){
			pos = i;
			break;
		}
	}
	if(pos != -1){
		printf("cache hit. need sector %d.\n",sectorNumber);
		cache[pos].lru = 0;
		for(int i = 0; i < 4; i++)
			if(i != pos && cache[i].valid == TRUE)
				cache[i].lru++;
		bcopy(cache[pos].data,data,SectorSize);

	}
	else{
		printf("cache miss. need sector %d.\n",sectorNumber);
		disk->ReadRequest(sectorNumber,data);
		semaphore->P();
			
		int swap = -1;
		for(int i = 0; i < 4; i++)
			if(cache[i].valid == FALSE){
				swap = i;
				break;
			}
		if(swap == -1){
			swap = 0;
			for(int i = 1; i <4; i++)
				if(cache[i].lru > cache[swap].lru)
					swap = i;
		}
		cache[swap].valid = TRUE;
		cache[swap].dirty = FALSE;
		cache[swap].sector = sectorNumber;
		bcopy(data,cache[swap].data,SectorSize);

	}

*/

    disk->ReadRequest(sectorNumber, data);

    semaphore->P();		

    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
/*
	for(int i = 0; i < 4; i++)
		if(cache[i].valid == TRUE && cache[i].sector == sectorNumber)
			cache[i].valid = FALSE;
*/
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{ 
    semaphore->V();
}

void
SynchDisk::plusReader(int sector){
	readerLock->Acquire();
	numReader[sector]++;
	if(numReader[sector] == 1)
		mutex[sector]->P();
//	printf("reader cnt: %d\n",numReader[sector]);
	readerLock->Release();
}

void
SynchDisk::minusReader(int sector){
	readerLock->Acquire();
	numReader[sector]--;
	if(numReader[sector] == 0)
		mutex[sector]->V();
//	printf("reader cnt: %d\n",numReader[sector]);
	readerLock->Release();
}

void
SynchDisk::startWrite(int sector){
	mutex[sector]->P();
}

void
SynchDisk::endWrite(int sector){
	mutex[sector]->V();
}
