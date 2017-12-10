// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
newThread(){
	printf("The new thread is running !!!\n");
	machine->Run();
	
	
}
/*
void
StartProcess(char *filename)
{
	
    OpenFile *executable = fileSystem->Open("../test/sort");
    OpenFile *executable_new = fileSystem->Open(filename);
    AddrSpace *space;
    AddrSpace *space_new;
    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    printf("Init old thread.\n");
    space = new AddrSpace(executable,"../test/sort");
    printf("Init new thread.\n");
    space_new = new AddrSpace(executable_new,filename);
    Thread *thread_new = new Thread("new thread");
    
    currentThread->space = space;
	printf("currentThread %s",currentThread->getName());
    space_new->InitRegisters();
    space_new->RestoreState();
    thread_new->space = space_new;
    
    thread_new->Fork(newThread,0);
    currentThread->Yield();
	
    delete executable;			
    delete executable_new;
    space->InitRegisters();		
    space->RestoreState();		

	printf("the new thread suspend now !.\n");
	thread_new->Suspend();
	printf("The old thread starts.\n");
    machine->Run();			

    
    ASSERT(FALSE);			
}
*/

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
		printf("Unable to open file %s\n", filename);
		return;
    }
     
	printf(" start process filename %s.\n",filename);
	space = new AddrSpace(executable,filename);
    currentThread->space = space;

    delete executable;			

    space->InitRegisters();		
    space->RestoreState();		

    machine->Run();			
    
    ASSERT(FALSE);			
}


// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;

static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

//    synchconsole = new SynchConsole(in, out);
	readAvail = new Semaphore("read avail", 0);
	writeDone = new Semaphore("write done", 0);
    
    for (;;) {
		readAvail->P();		// wait for character to arrive
		ch = console->getChar();
		console->putChar(ch);	// echo it!
		writeDone->P() ;        // wait for write to finish
		if (ch == 'q') return;  // if q, quit
    }
}
