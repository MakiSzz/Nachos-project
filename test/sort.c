/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

//int A[1024];	/* size of physical memory; with code, we'll run out of space!*/
int A[100];
/*
void
func(){
	
	Create("test1.txt");
}
*/
int
main()
{

//	Exit(0);
//	Create("test2.txt");
//	Fork(func);
    int i, j, tmp;
//	int id = Exec("../test/halt");
//	Join(id);
   

    for (i = 0; i < 100; i++)		
        A[i] = 100 - i;

  

    for (i = 0; i < 100; i++)
        for (j = 0; j < (100 - i); j++)
	   if (A[j] > A[j + 1]) {	
	      tmp = A[j];
	      A[j] = A[j + 1];
	      A[j + 1] = tmp;
    	   }
    Exit(A[0]);
}
