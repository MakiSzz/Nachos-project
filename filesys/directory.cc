// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
	
    for (int i = 0; i < tableSize; i++)
		table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
	    return i;
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector)
{ 
    if (FindIndex(name) != -1)
	return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
			table[i].name = name;
//          strncpy(table[i].name, name, FileNameMaxLen); 
            table[i].sector = newSector;
        return TRUE;
	}
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);

    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse)
	    printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}

void
Directory::recordName(char *name){
	int i;
	if(strcmp(name,"..") == 0){
		if((i = FindIndex(table[0].fname)) != -1){
			printf("i = %d.\n",i);
			table[0].fname = table[i].fname;
			table[1].fname = table[i].fname;
		}
	}
	else{
		table[0].fname = name;
		table[1].fname = name;
	}
}

void
Directory::getPath(char *path){
	char *name[10];
	int i;
	int index = 0;
	for(i = 0; i < 10; i++){
		if(strcmp(table[index].fname,"root") == 0)
			break;
		name[i] = table[index].fname;
		index = FindIndex(table[index].fname);
	}
	if(i != 0)
		i--;
	for(i; i >= 0; i--){
		strcat(path,"/");
		strcat(path,name[i]);
	}
	strcat(path,'\0');
}
char*
Directory::printName(){
	return table[0].fname;
}
void
Directory::listFile(char *filename){
	int i;
	if((i = FindIndex(filename)) != -1){
		printf("filesize %d, filetype %d,fname %s\n",table[i].fileSize,table[i].fileType,table[i].fname);
	}
}
/*
int
Directory::getCDSector(char *dirPath){
	int directorySector = -1;
	char *root = "root\/";
	int test = strcmp(root,dirPath);
	if(test == 0){
		return 1;
	}
	else{
		int index = Find(dirPath);
		if(index != -1){
			printf("find the directoryPath in the root directory the table.sector is %d.\n",index);
			return index;
		}
		else{
			for(int i = 0; i < tableSize; i++){
				if(table[i].inUse && (table[i].fileType == dirType)){
					int dirIndex = table[i].sector;
					OpenFile *tempOpenFile = new OpenFile(directoryIndex);
					Directory *directory = new Directory(200);
					directory->FetchForm(tempOpenFile);
					directorySector = directory->getCDSector(table[i].absPath);
					delete tempOpenFile;
					delete directory;
					return directorySector;
				}
			}
		}
	}
	return directorySector;
}

bool
Directory::AddDirectory(char *absPath,char *dirPath,int newSector){
	if(FindIndex(absPath) != -1)
		return FALSE;
	for(int i = 0; i < tableSize; i++){
		if(!table[i].inUse){
			table[i].inUse = TRUE;
			table[i].absPath = absPath;
			table[i].dirPath = airPath;
			table[i].Sector = newSector;
			table[i].fileType = DirType;
			return TRUE;
		}
		
	}
	return FALSE;
}
*/
