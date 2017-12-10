// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "addrspace.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
exec_func(int address){
		char fileName[100];
		int pos = 0;
		int data;
		while(1){
			machine->ReadMem(address+pos,1,&data);
			if(data == 0){
				fileName[pos]='\0';
				break;
			}
			fileName[pos++] = (char)data;
			//printf(" %d %c\n",pos-1,fileName[pos-1]);
		}
		fileName[0] = '.';
		printf("fileName %s.\n",fileName);
		OpenFile *executable = fileSystem->Open(fileName);
		AddrSpace *space;

		if (executable == NULL) {
			printf("AUnable to open file  %s\n", fileName);
			return;
		}
	    space = new AddrSpace(executable,fileName);    
	    currentThread->space = space;

	    delete executable;			

	    space->InitRegisters();		
	    space->RestoreState();		

	    machine->Run();
}

void
fork_func(int address){
	Info *info = (Info*)address;

	AddrSpace *space = new AddrSpace;
	space->AddrSpaceCopy(info->space);
	currentThread->space = space;
	int currentPc = info->pc;
	space->InitRegisters();
	space->RestoreState();
	machine->WriteRegister(PCReg,currentPc);
	machine->WriteRegister(NextPCReg,currentPc+4);
//	currentThread->SaveUserState();
//	printf("reach here.\n");
	machine->Run();
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
		DEBUG('a', "Shutdown, initiated by user program.\n");
		printf("Entered here.\n");
  	 	interrupt->Halt();
    } 
	else if((which == SyscallException) && (type == SC_Help)){
		printf("\n\n");
		printf("Usage:	x <userprogram name>	:execute a user program\n");
		printf("	pwd			:show the current path\n");
		printf("	ls			:show the content in the current directory\n");
		printf("	cd <path>		:change the working path\n");
		printf("	cf <fileName>		:create a new file\n");
		printf("	rm <fileName>		:delete a file\n");
		printf("	mkdir <directoryName>	:create a new directory\n");
		printf("	rmdir <directoryName>	:remove a directory\n");
		printf("	q			:quit the shell program\n\n");
		
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_RDir)){
		int address = machine->ReadRegister(4);
		int value;
		int count = 0;
		do{
			machine->ReadMem(address++,1,&value);
			count++;
		}while(value != 0);
		printf("filename length is %d.\n",count);
		address = address - count;
		char fileName[count];
		for(int i = 0; i < count; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
		printf("fileName: %s.\n",fileName);
		rmdir(fileName);
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_CDir)){
		int address = machine->ReadRegister(4);
		int value;
		int count = 0;
		do{
			machine->ReadMem(address++,1,&value);
			count++;
		}while(value != 0);
		printf("filename length is %d.\n",count);
		address = address - count;
		char fileName[count];
		for(int i = 0; i < count; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
		printf("fileName: %s.\n",fileName);
		mkdir(fileName,00777);
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Remove)){
		int address = machine->ReadRegister(4);
		int value;
		int count = 0;
		do{
			machine->ReadMem(address++,1,&value);
			count++;
		}while(value != 0);
		printf("filename length is %d.\n",count);
		address = address - count;
		char fileName[count];
		for(int i = 0; i < count; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
		printf("fileName: %s.\n",fileName);
		fileSystem->Remove(fileName);
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Cd)){
		int address = machine->ReadRegister(4);
		int value;
		int count = 0;
		do{
			machine->ReadMem(address++,1,&value);
			count++;
		}while(value != 0);
		printf("filename length is %d.\n",count);
		address = address - count;
		char fileName[count];
		for(int i = 0; i < count; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
		printf("fileName: %s.\n",fileName);
		chdir(fileName);
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Ls)){
		system("ls");
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Pwd)){
		system("pwd");
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Exec)){
		printf("system call exec. \n");
		int address = machine->ReadRegister(4);
		Thread *newThread = new Thread("second thread");
		newThread->Fork(exec_func,address);
		machine->WriteRegister(2,newThread->get_tid_());
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Fork)){
		printf("system call fork.\n");
		int functionPc = machine->ReadRegister(4);
/*
		OpenFile *executable = fileSystem->Open(currentThread->filename);
		AddrSpace *space = new AddrSpace(executable);
		space = currentThread->space;
*/
		Info *info = new Info;
		info->space = currentThread->space;
		info->pc = functionPc;
		Thread* newThread = new Thread("second thread");
		printf("reach here2.\n");

		newThread->Fork(fork_func,int(info));
		
		machine->PcAdvance();
		
	}
	else if((which == SyscallException) && (type == SC_Yield)){
		printf("system call yield.%s\n",currentThread->getName());
		machine->PcAdvance();
		currentThread->Yield();
	}
	else if((which == SyscallException) && (type == SC_Join)){
		printf("system call join.\n");
		int threadId = machine->ReadRegister(4);
		while(AvaNum[threadId] == 1)
			currentThread->Yield();
		machine->PcAdvance();
	}	
	else if((which == SyscallException) && (type == SC_Exit)){
		printf("Program Exit. thread name %s\n",currentThread->getName());

		for(int j = 0; j < machine->pageTableSize; j++)
			if(machine->pageTable[j].valid == TRUE ){
				machine->pageTable[j].valid = FALSE;
				if(machine->memBitMap->Test(machine->pageTable[j].physicalPage))
					machine->memBitMap->Clear(machine->pageTable[j].physicalPage);	
				}
		//currentThread->Finish();	
/*
		int NextPC = machine->ReadRegister(NextPCReg);
		machine->WriteRegister(PCReg,NextPC);
*/
		int status = machine->ReadRegister(4);
		printf("thread exit with status %d.\n",status);
		machine->PcAdvance();
		currentThread->Finish();
	
	}      
	else if((which == SyscallException) && (type == SC_Create)){
		printf("system call create.\n");
		int address = machine->ReadRegister(4);
		int value;
		int count = 0;
		do{
			machine->ReadMem(address++,1,&value);
			count++;
		}while(value != 0);
		printf("filename length is %d.\n",count);
		address = address - count;
		char fileName[count];
		for(int i = 0; i < count; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
		printf("fileName: %s.\n",fileName);
		fileSystem->Create(fileName,128);
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Open)){
		printf("system call open.\n");
		int address = machine->ReadRegister(4);
		int value;
		int count = 0;
		do{
			machine->ReadMem(address++,1,&value);
			count++;
		}while(value != 0);
		printf("filename length is %d.\n",count);
		address = address - count;
		char fileName[count];
		for(int i = 0; i < count; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
			currentThread->filename[i] = fileName[i];
		}
		
		OpenFile *openfile = fileSystem->Open(fileName);
		machine->WriteRegister(2,int(openfile));
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Close)){
		printf("system call close.\n");
		int fd = machine->ReadRegister(4);
		OpenFile *openfile = (OpenFile*)fd;

		delete openfile;
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Write)){
		printf("system call write.\n");
		int address = machine->ReadRegister(4);
		int count = machine->ReadRegister(5);
		int fd = machine->ReadRegister(6);
		char data[count];
		int value;
		for(int i = 0; i < count; i++){
			machine->ReadMem(address+i,1,&value);
			data[i] = char(value);
		}
		if(fd != 1){
			OpenFile *openfile = (OpenFile*)fd;
			openfile->Write(data,count);
		}
		else{
			for(int i = 0; i < count; i++)
				putchar(data[i]);
		}
		machine->PcAdvance();
	}
	else if((which == SyscallException) && (type == SC_Read)){
		//printf("system call read.\n");
		int address = machine->ReadRegister(4);
		int count = machine->ReadRegister(5);
		int fd = machine->ReadRegister(6);
		char data[count];
		int result;
		if(fd != 0){
			OpenFile *openfile = (OpenFile*)fd;
			
			result = openfile->Read(data,count);
			
		}
		else{
			for(int i = 0; i < count; i++)
				data[i] = getchar();
			result = count;
			
		}
		for(int i = 0; i < result; i++){
			machine->WriteMem(address+i,1,int(data[i]));
		}
		machine->WriteRegister(2,result);
		machine->PcAdvance();
	}
	else if(which == PageFaultException){
		if(machine->tlb != NULL){
			int vpn = (unsigned) machine->registers[BadVAddrReg]/PageSize;
			int position = -1;
			for(int i = 0; i < TLBSize; i++){
				if(machine->tlb[i].valid == FALSE){
					position = i;
					break;
				}
			}
//------------------------------FIFO
/*
			if(position == -1){
				position = 0;
				for(int i = TLBSize - 1; i > 0; i--){
					machine->tlb[i] = machine->tlb[i-1];
				}
			}
*/
//------------------------------LRU

			if(position == -1){
				position = machine->currentTLB;
				machine->LRU_queue[position] = 0;
				for(int k = 0; k < TLBSize; k++){
						machine->LRU_queue[k]++;
						if(machine->LRU_queue[k] > machine->LRU_queue[machine->currentTLB])
							machine->currentTLB = k;
				}
			}
			else{
				machine->LRU_queue[position] = 0;
				for(int k = 0; k < TLBSize; k++){
					if(machine->tlb[k].valid){
						machine->LRU_queue[k]++;
						if(machine->LRU_queue[k] > machine->LRU_queue[machine->currentTLB])
							machine->currentTLB = k;
					}
				}
			}


		
			machine->tlb[position].valid = true;
			machine->tlb[position].virtualPage = vpn;


			if(machine->pageTable[vpn].valid == FALSE){
				OpenFile *openfile;
				if(strcmp(machine->fileName,"../test/shell") == 0)
					openfile = fileSystem->Open("../userprog/temp1");
				else
					openfile = fileSystem->Open("../userprog/temp2");
				if(openfile == NULL) ASSERT(FALSE);
				int vpn = (unsigned) machine->registers[BadVAddrReg]/PageSize;
				int position = machine->memBitMap->Find();
				if(position == -1){
					//position = 0;
					for(int j = 0; j < machine->pageTableSize; j++){
						if(machine->pageTable[j].valid == TRUE){
						if(machine->pageTable[j].dirty == TRUE)
							openfile->WriteAt(&(machine->mainMemory[j*PageSize]),PageSize,machine->pageTable[j].virtualPage*PageSize);
							machine->pageTable[j].valid = FALSE;
							position = j;
							break;
						}
					}
				}
		//		printf("page fault vpn %d @ position %d.\n",vpn,position);
				openfile->ReadAt(&(machine->mainMemory[position*PageSize]),PageSize,vpn*PageSize);
				machine->pageTable[vpn].valid = TRUE;
				machine->pageTable[vpn].physicalPage = position;
				
				machine->pageTable[vpn].use = FALSE;
				machine->pageTable[vpn].dirty = FALSE;
				machine->pageTable[vpn].readOnly = FALSE;
				delete openfile;
			}
	

			machine->tlb[position].physicalPage = machine->pageTable[vpn].physicalPage;
			machine->tlb[position].use = false;
			machine->tlb[position].dirty = false;
			machine->tlb[position].readOnly = false;

		}
		else{
			OpenFile *openfile = fileSystem->Open("temp_mem");
			if(openfile == NULL) ASSERT(FALSE);
		//	printf("%d\n",machine->registers[BadVAddrReg]);
			int vpn = (unsigned) machine->registers[BadVAddrReg]/PageSize;
			int position = machine->memBitMap->Find();
		//	printf("position %d  vpn %d\n",position,vpn);
			if(position == -1){
				position = 31;
				if(machine->pageTable[0].valid == TRUE && machine->pageTable[0].dirty == TRUE){
						openfile->WriteAt(&(machine->mainMemory[0]),PageSize,machine->pageTable[0].virtualPage*PageSize);
						machine->pageTable[0].valid = FALSE;
				}
				for(int j = 0;j < machine->pageTableSize - 1;j++){
					machine->pageTable[j] = machine->pageTable[j+1];
					machine->pageTable[j].physicalPage = j;
				}
			}
		//	printf("vpn %d, position %d\n",vpn,position);
			openfile->ReadAt(&(machine->mainMemory[position*PageSize]),PageSize,vpn*PageSize);
			machine->pageTable[position].valid = TRUE;
			machine->pageTable[position].threadId = currentThread->get_tid_();
			machine->pageTable[position].physicalPage = position;
			machine->pageTable[position].virtualPage = vpn;
			machine->pageTable[position].use = FALSE;
			machine->pageTable[position].dirty = FALSE;
			machine->pageTable[position].readOnly = FALSE;
			printf("page fault happened vpn %d, thread Id %d.\n",vpn,currentThread->get_tid_());
			delete openfile;


		}
	}
    else {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}
