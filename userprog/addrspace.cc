// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------
AddrSpace::AddrSpace(){
	pageTable = NULL;
}
AddrSpace::AddrSpace(OpenFile *executable,char *filename)
{
    NoffHeader noffH;
    unsigned int i, size;
	fileName = filename;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

//    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation
/*
	pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	
	pageTable[i].physicalPage = i;
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  
    }
*/

    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	
	pageTable[i].physicalPage = -1;
	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;
    }
 
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
// multi thread 
//	bzero(machine->mainMemory, size);
//orignal code
/*
	if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
*/
//	printf("addrspace filename %s.\n",filename);
	OpenFile *openfile;
	printf("filename %s\n",filename);
	if(strcmp(filename,"../test/shell") == 0){
		
		fileSystem->Create("temp1",12800);
    	openfile = fileSystem->Open("temp1");
	}
	else{
		fileSystem->Create("temp2",12800);
		openfile = fileSystem->Open("temp2");
	}
    if(openfile == NULL) ASSERT(FALSE);
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
	
	int code_pos = noffH.code.inFileAddr;
	int code_pos_new = noffH.code.virtualAddr;
	char current_char;
	for(int j = 0; j < noffH.code.size; j++){
		
		executable->ReadAt(&(current_char),1, code_pos++);
		openfile->WriteAt(&(current_char),1,code_pos_new++);
	}

    }
    
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
	int data_pos = noffH.initData.inFileAddr;
	int data_pos_new = noffH.initData.virtualAddr;
	char current_char;
	for(int j = 0; j < noffH.initData.size; j++){
		
	        executable->ReadAt(&(current_char),1, data_pos++);
		openfile->WriteAt(&(current_char),1,data_pos_new++);
	

	}
    }
    delete openfile;

// then, copy in the code and data segments into memory
/*
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
	
	int code_pos = noffH.code.inFileAddr;
	
	for(int j = 0; j < noffH.code.size; j++){
		int vpn_tmp = (noffH.code.virtualAddr + j)/PageSize;
		int offset_tmp = (noffH.code.virtualAddr + j)%PageSize;
		int paddr = pageTable[vpn_tmp].physicalPage*PageSize + offset_tmp;
		executable->ReadAt(&(machine->mainMemory[paddr]),
			1, code_pos++);
	}
	executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),noffH.code.size, noffH.code.inFileAddr);
    }


    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);

	int data_pos = noffH.initData.inFileAddr;
	for(int j = 0; j < noffH.initData.size; j++){
		int vpn_tmp = (noffH.initData.virtualAddr + j)/PageSize;
		int offset_tmp = (noffH.initData.virtualAddr + j)%PageSize;
		int paddr = pageTable[vpn_tmp].physicalPage*PageSize+offset_tmp;
		
	        executable->ReadAt(&(machine->mainMemory[paddr]),1, data_pos++);

	}

	executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),noffH.initData.size, noffH.initData.inFileAddr);	
    }
*/
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
		machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{

	for(int i = 0; i < TLBSize; i++){
		machine->tlb[i].valid = FALSE;
		machine->LRU_queue[i] = 1;
	}
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
	machine->fileName = fileName;
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
void AddrSpace::AddrSpaceCopy(AddrSpace* tspace){

	numPages = tspace->numPages;
	pageTable = new TranslationEntry[numPages];
	for (int i = 0; i < numPages; i++) {
		pageTable[i] = tspace->pageTable[i];
    }
}
Info::Info(){
	space = NULL;
}
Info::~Info(){
	
}
