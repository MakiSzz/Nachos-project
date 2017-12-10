// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "system.h"
// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)
#define Pipesector 100
#define PipeFileSize 500
//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
	currentPath = new char[100];
	currentPath[0] = 'r';
	currentPath[1] = 'o';
	currentPath[2] = 'o';
	currentPath[3] = 't';
	currentPath[4] = '\0';
        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
	FileHeader *mapHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;
	FileHeader *pipeHdr = new FileHeader;
	freeMap->Mark(Pipesector);
        DEBUG('f', "Formatting the file system.\n");
	
    // First, allocate space for FileHeaders for the directory and bitmap
    // (make sure no one else grabs these!)
	freeMap->Mark(FreeMapSector);	    
	freeMap->Mark(DirectorySector);

    // Second, allocate space for the data blocks containing the contents
    // of the directory and bitmap files.  There better be enough space!

	ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
	ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));
	ASSERT(pipeHdr->Allocate(freeMap,PipeFileSize));
    // Flush the bitmap and directory FileHeaders back to disk
    // We need to do this before we can "Open" the file, since open
    // reads the file header off of disk (and currently the disk has garbage
    // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
	mapHdr->WriteBack(FreeMapSector);    
	dirHdr->WriteBack(DirectorySector);
	pipeHdr->WriteBack(Pipesector);
    // OK to open the bitmap and directory files now
    // The file system operations assume these two files are left open
    // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
     
    // Once we have the files "open", we can write the initial version
    // of each file back to disk.  The directory at this point is completely
    // empty; but the bitmap has been changed to reflect the fact that
    // sectors on the disk have been allocated for the file headers and
    // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
	freeMap->WriteBack(freeMapFile);	 // flush changes to disk
	directory->WriteBack(directoryFile);

	if (DebugIsEnabled('f')) {
	    freeMap->Print();
	    directory->Print();

        delete freeMap; 
	delete directory; 
	delete mapHdr; 
	delete dirHdr;
	}
    } else {
    // if we are not formatting the disk, just open the files representing
    // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize,int fileType)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
//----------------------------------------------------------------
	int mysector = directoryFile;
	if(strcmp(currentPath,"root") != 0){
		int i = 5;
		int len = strlen(currentPath);
		char *temp = new char[20];
		temp[0] = '\0';
		int pos = 0;
		while(currentPath[i] != '\0'){
			if(currentPath[i] != '/')
				temp[pos++] = currentPath[i++];
			else{
				temp[pos] = '\0';
				int j = directory->FindIndex(temp);
				delete directory;
				Directory *directory = new Directory(NumDirEntries);
				directory->FetchFrom(j);
				mysector = j;
				delete temp;
				char *temp = new char[20];
				temp[0] = '\0';
				pos = 0;
				i++;
			}
		}
		if(currentPath[i] == '\0' && temp[0] != '\0'){
			int j = directory->FindIndex(temp);
			delete directory;
			Directory *directory = new Directory(NumDirEntries);
			directory->FetchFrom(j);
			mysector = j;
		}
	}
//----------------------------------------------------------------
    if (directory->Find(name) != -1)
      success = FALSE;			// file is already in directory
    else {	
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();	// find a sector to hold the file header
    	if (sector == -1) 		
            success = FALSE;		// no free block for file header 
        else if (!directory->Add(name, sector))
            success = FALSE;	// no space in directory
		else {
//---------------------------------------------------
			int i = directory->FindIndex(name);
			directory->table[i].fileType = fileType;
//---------------------------------------------------
    	    hdr = new FileHeader;
			if(fileType == 1){
	 	    	if (!hdr->Allocate(freeMap, initialSize))
     	    		success = FALSE;	// no space on disk for data
				else {	
	    			success = TRUE;

			// everthing worked, flush all changes back to disk
    		    	hdr->WriteBack(sector); 		
    		    	directory->WriteBack(directoryFile);
    		    	freeMap->WriteBack(freeMapFile);
	    		}
			}
			else{
				if (!hdr->Allocate(freeMap, DirectoryFileSize))
     	    		success = FALSE;	// no space on disk for data
				else {	
	    			success = TRUE;

			// everthing worked, flush all changes back to disk
    		    	hdr->WriteBack(sector); 		
    		    	directory->WriteBack(mysector);
    		    	freeMap->WriteBack(freeMapFile);
	    		}
			}
            delete hdr;
		}
        delete freeMap;
    }
    delete directory;
    return success;
}

/*
bool
FileSystem::createDir(char *absPath,char *dirPath){
	Directory *directory;
	BitMap *freeMap;
	FileHeader *hdr;
	int sector;
	bool success;
	int correctSector;
	OpenFile *dirFile;

	directory = new Directory(NumDirEntries);
	directory->FecthForm(directoryFile);

	correctSector = directory->getCDSector(dirPath);
	printf("now the correctSector is %d\n",correctSector);
	if(correctSector == -1){
		printf("can not find the directoryPath. \n");
		success = FALSE;
	}
	else{
		printf("find the directory path.\n");
		if(correctSector != 1){
			printf("reset the directory.\n");
			delete directory;
			theDirectoryFile = new OpenFile(correctSector);
			directory = new Directory(NumDirEntries);
			directory->FecthForm(theDirectoryFile);
		}
		if(directory->Find(absPath) != -1){
			printf("in the root directory already have the abs Path.\n");
			success = FALSE;
		}
		else{
			freeMap = new BitMap(NumSectors);
			freeMap->FecthFrom(freeMapFile);
			sector = freeMap->Find();
			if(sector == -1){
				success = FALSE;
				printf("no free block .\n");
			}
			else if(directory->AddDirectory(absPath,dirPath,sector)){
				printf("no space in dir.\n");
				success = FALSE;
			}
			else{
				hdr = new FileHeader;
				if(!hdr->Allocate(freeMap,DirectorySize)){
					printf("no space on disk.\n");
					success = FALSE;
				}
				else{
					success = TRUE;
					printf("now success.\n");
					hdr->WriteBack(sector);
					directory->WriteBack(directoryFile);
					freeMap->WriteBack(freeMapFile);
					OpenFile *newDirectoryFile = new OpenFile(sector);
					Directory *newDirectory = new Directory(NumDirEntries);
					newDirectory->WriteBack(newDirectoryFile);
					delete newDirectory;
					delete newDirectoryFile;
				}
				delete hdr;	
			}
			delete freeMap;
		}
	}
	return success;
}
*/
//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{ 
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Opening file %s\n", name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name); 
    if (sector >= 0) 		
	openFile = new OpenFile(sector);	// name was found in directory 
    delete directory;
    return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(char *name)
{ 
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1) {
       delete directory;
       return FALSE;			 // file not found 
    }

	if(synchDisk->numVis[sector]){
		printf("unable to remove the file, still someone using it.\n");
		return FALSE;
	}
	printf("remove success! currentThread is %s.\n",currentThread->getName());
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(directoryFile);        // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
} 

void
FileSystem::changePWD(char *name){

	if(strcmp(name,"..") == 0){
		if(strcmp(currentPath,"root") != 0){
			int len = strlen(currenPath);
			for(int i = len - 1; i > 0; i--){
				if(currentPath[i] == '/'){
					currentPath[i] = '\0';
					break;
				}
			}
		}
	}
	else{
		Directory *directory = new Directory(NumDirEntries);
		directory->FetchFrom(directoryFile);
		if(strcmp(currentPath,"root") != 0){
			int i = 5;
			int len = strlen(currentPath);
			char *temp = new char[20];
			temp[0] = '\0';
			int pos = 0;
			while(currentPath[i] != '\0'){
				if(currentPath[i] != '/')
					temp[pos++] = currentPath[i++];
				else{
					temp[pos] = '\0';
					int j = directory->FindIndex(temp);
					delete directory;
					Directory *directory = new Directory(NumDirEntries);
					directory->FetchFrom(j);
					delete temp;
					char *temp = new char[20];
					temp[0] = '\0';
					pos = 0;
					i++;
				}
			}
			if(currentPath[i] == '\0' && temp[0] != '\0'){
				int j = directory->FindIndex(temp);
				delete directory;
				Directory *directory = new Directory(NumDirEntries);
				directory->FetchFrom(j);
			}
		}
		if((i = directory->FindIndex(name)) != -1){
			strcat(currentPath,'/');
			strcat(currentPath,name);
			delete directory;
		}
		else{
			printf("no such dir.\n");
			ASSERT(FALSE);
		}
	}
}


void
FileSystem::listOne(char *filename){
	Directory *directory = new Directory(NumDirEntries);
	directory->FetchFrom(directoryFile);
	directory->listFile(filename);
	delete directory;
}
bool
FileSystem::extendFile(char *name,int extrasize){
	Directory *directory = new Directory(NumDirEntries);
	directory->FetchFrom(directoryFile);
	int fileHeadSector = directory->Find(name);
	if(fileHeadSector == -1){
		return FALSE;	
	}
	FileHeader *fileHdr;
	fileHdr = new FileHeader;
	fileHdr->FetchFrom(fileHeadSector);
	BitMap *freeMap;
	freeMap = new BitMap(NumSectors);
	freeMap->FetchFrom(freeMapFile);
	fileHdr->extendLen(freeMap,extrasize);
	fileHdr->WriteBack(fileHeadSector);
	return TRUE;
}

void
FileSystem::writePipe(char *data,int length){
	OpenFile *pipe = new OpenFile(Pipesector);
	pipe->Write(data,length);
	printf("write data into the file.\n");
	delete pipe;
	FileHeader *filehdr  = new FileHeader;
	filehdr->FetchFrom(Pipesector);
	filehdr->numBytes = length;
	filehdr->WriteBack(Pipesector);
}

int
FileSystem::readPipe(char *data){
	FileHeader *filehdr = new FileHeader;
	filehdr->FetchFrom(Pipesector);
	int length = filehdr->numBytes;
	filehdr->WriteBack(Pipesector);
	OpenFile *pipe = new OpenFile(Pipesector);
	pipe->Read(data,length);
	printf("read from pipe.\n");
	delete pipe;
	return length;
}
