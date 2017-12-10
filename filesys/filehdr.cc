// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "time.h"
#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
		return FALSE;		// not enough space
	setCreateTime();
	printf("now in fileheader::allocate! filesize %d numBytes %d  numSectors %d createTime %s.\n",fileSize,numBytes, numSectors,createTime);
//	printf("numDirect %d\n",NumDirect);
	if(numSectors < NumDirect){
/*
		dataSectors[0] = freeMap->myFind(numSectors);
		if(dataSectors[0] == -1)
		{*/
    		for (int i = 0; i < numSectors; i++){
				dataSectors[i] = freeMap->Find();
				printf("dataSectors %d freemap %d.\n",i,dataSectors[i]);	
			}
	/*	}
		else{
			printf("allocate by myFind numSectors %d.\n",numSectors);
			for(int i = 1; i < numSectors; i++){
				dataSectors[i] = dataSectors[i-1]+1;
				printf("dataSectors %d freemap %d.\n",i,dataSectors[i]);
			}
		}*/
	}

	else{
		for(int i = 0; i < NumDirect; i++){
			dataSectors[i] = freeMap->Find();
			printf("dataSectors %d freemap %d.\n",i,dataSectors[i]);
		}
		int leftSectors = numSectors - NumDirect;
//		printf("leftSectors %d secondDirect %d.\n",leftSectors,SecondDirect);
//		printf("leftSectors / SecondDirect.%d\n",leftSectors/SecondDirect);
		int needSecondIndex = (leftSectors/32) + (leftSectors%32+31)/32;
		printf("need second index %d.\n",needSecondIndex);
		for(int i = 0; i < needSecondIndex; i++){
			dataSectors[NumDirect + i] = freeMap->Find();
			int num;
			int test = leftSectors - SecondDirect;
			num = (test > 0) ? SecondDirect : leftSectors;
			int *sectors = new int[num];
			for(int j = 0; j < num; j++){
				sectors[j] = freeMap->Find();
				printf("second index %d sectors %d freemap %d.\n",i,j,sectors[j]);
			}
			synchDisk->WriteSector(dataSectors[NumDirect+i],(char*)sectors);
			delete sectors;
			leftSectors = leftSectors - 32;
		}
	}

    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
	if(numSectors < NumDirect){
    	for (int i = 0; i < numSectors; i++) {
			ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
			freeMap->Clear((int) dataSectors[i]);
    	}
	}

	else{
		for(int i = 0; i < NumDirect; i++){
			ASSERT(freeMap->Test((int) dataSectors[i]));
			freeMap->Clear((int) dataSectors[i]);
		}
		int leftSectors = numSectors - NumDirect;
		int needSecondIndex = leftSectors/32 + (31+leftSectors%32)/32;
		for(int i = 0; i < needSecondIndex; i++){
			char *sectorTemp = new char[SectorSize];
			synchDisk->ReadSector(dataSectors[NumDirect+i],sectorTemp);
			int *sector = (int*)sectorTemp;
			int num = (leftSectors > 32)?32:leftSectors;
			for(int j = 0; j < num; j++){
				ASSERT(freeMap->Test((int) sector[j]));
				freeMap->Clear((int) sector[j]);
			}
			delete sector;
			leftSectors -= num;
			ASSERT(freeMap->Test((int) dataSectors[NumDirect+i]));
			freeMap->Clear((int) dataSectors[NumDirect+i]);
		}
	}

}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
//	printf("now in fileheader::writeback! theSector %d.\n",sector);
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
	int sectorOffset = offset / SectorSize;
	if(sectorOffset < NumDirect)
	    return(dataSectors[offset / SectorSize]);

	else{
		int leftSector = sectorOffset - NumDirect;
		int index = (leftSector)/32;
		char *sectorTemp = new char[SectorSize];
		synchDisk->ReadSector(dataSectors[NumDirect+index],sectorTemp);
		int *sector = (int*)sectorTemp;
		int indexOffset = sectorOffset - NumDirect - index*32;
		int returnValue = sector[indexOffset];
		delete sector;

		return returnValue;
	}

}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
//	printf("theSctor %d type %s.\n",theSector,type);
	printf("createTime %s\nlastReadTime %s\nlastWriteTime %s\n",createTime,lastReadTime,lastWriteTime);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}

void
FileHeader::setCreateTime()
{

	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char *temp = asctime(timeinfo);
	strncpy(createTime,temp,24);

}

void
FileHeader::setLastReadTime()
{

	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char *temp = asctime(timeinfo);
	strncpy(lastReadTime,temp,24);

	
}
void
FileHeader::setLastWriteTime()
{

	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char *temp = asctime(timeinfo);
	strncpy(lastWriteTime,temp,24);

}

bool
FileHeader::extendLen(BitMap *bitMap,int fileSize){
	int needsectors = divRoundUp(fileSize,SectorSize);
	int originalByte = FileLength();
	int originalSectors = divRoundUp(originalByte,SectorSize);
	if(bitMap->NumClear() < needsectors){
		return FALSE;
	}
	printf("now extend!\n need sectors %d original sectors %d.\n",needsectors,numSectors);
	int totalsectors = needsectors + originalSectors;
	if(totalsectors > totalDirect){
		printf("the file is too big.\n");
		return FALSE;
	}
	if(totalsectors < NumDirect){
		printf("we only need the first .\n");
		for(int i =0; i < needsectors; i++){
			dataSectors[originalSectors+i] = bitMap->Find();
			printf("originalSectors+i %d, dataSectors[originalSectors+i] %d.\n",originalSectors+i,dataSectors[originalSectors+i]);
		}
	}
	else if(originalSectors <NumDirect){
		printf("we need the first and second.\n");
		int directLeftSectors = NumDirect - originalSectors;
		for(int i = 0; i < directLeftSectors; i++){
			dataSectors[originalSectors+i] = bitMap->Find();
			printf("originalSectors+i %d, dataSectors[originalSectors+i] %d.\n",originalSectors+i,dataSectors[originalSectors+i]);
		}
		int leftSectors = needsectors - directLeftSectors;
		int needSecond = divRoundUp(leftSectors,32);
		printf("need second %d",needSecond);
		for(int i = 0; i < needSecond; i++){
			dataSectors[NumDirect + i] = bitMap->Find();
			int num;
			int test = leftSectors - SecondDirect;
			num = (test > 0) ? SecondDirect : leftSectors;
			int *sectors = new int[num];
			for(int j = 0; j < num; j++){
				sectors[j] = bitMap->Find();
				printf("second index %d sectors %d bitmap %d.\n",i,j,sectors[j]);
			}
			synchDisk->WriteSector(dataSectors[NumDirect+i],(char*)sectors);
			delete sectors;
			leftSectors = leftSectors - 32;
		}
	}
	else{
		printf("only need second .\n");
		int originalSecond = (originalSectors - NumDirect)/32+((originalSectors - NumDirect)%32+31)/32;

		int temp =(originalSecond)*32;
		int lastSecond = originalSectors -temp - NumDirect;
	
		printf("the originalSecond %d, the lastsecond %d.\n",originalSecond,lastSecond);
		if(lastSecond == 0){
			int needSecond = divRoundUp(needsectors,32);
			for(int i = 0; i < needSecond; i++){
			dataSectors[NumDirect + originalSecond + i] = bitMap->Find();
			int num;
			int test = needsectors - SecondDirect;
			num = (test > 0) ? SecondDirect : needsectors;
			int *sectors = new int[num];
			for(int j = 0; j < num; j++){
				sectors[j] = bitMap->Find();
				printf("second index %d sectors %d bitmap %d.\n",i,j,sectors[j]);
			}
			synchDisk->WriteSector(dataSectors[NumDirect+originalSecond+i],(char*)sectors);
			delete sectors;
			needsectors = needsectors - 32;
			}
		}
		else{
			lastSecond+=32;
			char *sectorTemp = new char[SectorSize];
			synchDisk->ReadSector(dataSectors[NumDirect+originalSecond-1],sectorTemp);
			int *sector = (int*)sectorTemp;
			int length = needsectors + lastSecond;
			if(length <= 32){
				for(int i = 0; i < needsectors; i++){
					sector[lastSecond+i] = bitMap->Find();
					printf("the sector[%d] is %d.\n",lastSecond+i,sector[lastSecond+i]);
				}
			}
			else{
				int first = 32 - lastSecond;
				for(int i = 0; i < first; i++){
					sector[lastSecond+i] = bitMap->Find();
					printf("the sector[%d] is %d.\n",lastSecond+i,sector[lastSecond+i]);
				}
				int leftSectors = needsectors - first;
				int needSecondIndex = (leftSectors/32) + (leftSectors%32+31)/32;
				printf("need second index %d.\n",needSecondIndex);
				for(int i = 0; i < needSecondIndex; i++){
					dataSectors[NumDirect + originalSecond +i] = bitMap->Find();
					int num;
					int test = leftSectors - SecondDirect;
					num = (test > 0) ? SecondDirect : leftSectors;
					int *sectors = new int[num];
					for(int j = 0; j < num; j++){
						sectors[j] = bitMap->Find();
						printf("second index %d sectors %d bitmap %d.\n",i,j,sectors[j]);
					}
					synchDisk->WriteSector(dataSectors[NumDirect+originalSecond+i],(char*)sectors);
					delete sectors;
					leftSectors = leftSectors - 32;
				}
			}
		}
	}
}
