/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

/*
	int fd1,fd2;
	int result;
	char buffer[20];
	Create("write.txt");
	fd1= Open("read.txt");
	fd2 = Open("write.txt");
	result = Read(buffer,20,fd1);
	Write(buffer,result,fd2);
	Close(fd1);
	Close(fd2);
*/
#include "syscall.h"

int
main()
{
//	Yield();
	Exit(0);
}
