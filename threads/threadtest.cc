// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "synch.h"
// testnum is set in main.cc
int testnum = 1;
Semaphore semNempty("empty",6);
Semaphore semNfull("full",0);
Semaphore sempool[6];
bool pool[6] = {0};
Lock *lock;
//---------------------------------------------
Condition *condition;
int countnum = 0;
void barrierTest(int n){
	lock->Acquire();
	countnum++;
	if(countnum == 3){
		printf("countnum: %d,Broadcast.\n",countnum);
		condition->Broadcast(lock);
		lock->Release();
	}
	else{
		printf("countnum: %d, Wait.\n",countnum);
		condition->Wait(lock);
		lock->Release();
	}
	printf("currentThread: %s, continue to run...\n",currentThread->getName());
	
}
//-------------------------------------
int pronum = 0;
class Production{
public:
	int id;
	Production(){
		id = pronum;
		pronum++;
	}
	~Production(){}
};
List *proList;

void produce2(int n){
	
	for(int i = 0; i <10; i++){
		lock->Acquire();
		Production *pro = new Production();
		proList->Append(pro);
		printf("thread: %s, produced production: %d\n",currentThread->getName(),pro->id);
		lock->Release();
	}
}

void consume2(int n){
	for(int i = 0; i < 10; i++){
		lock->Acquire();
		if(proList->IsEmpty()){
			printf("thread:%s, no production\n",currentThread->getName());
			lock->Release();
			i--;
			currentThread->Yield();
		}
		else{
			Production *pro = (Production*)proList->Remove();
			printf("thread: %s,consume production: %d\n",currentThread->getName(),pro->id);
			delete pro;
			lock->Release();
		}
	}
}
//---------------------------------------
void
Produce(int arg)
{
	while(1){
		semNempty.P();
		int i,j;
		for(i = 0; i < 6; i++){
			sempool[i].P();
			if(!pool[i]){
				pool[i] = true;
				printf("%s produces %d | ",currentThread->getName(),i);
				for(j = 0; j < 6; j++)
					printf(" %d",pool[j]);
				printf("\n");
				sempool[i].V();
				currentThread->Yield();
				break;
			}
			else{
				sempool[i].V();
			}	
		}
		semNfull.V();
	}
}

void
Consume(int arg)
{
	while(1){
		semNfull.P();
		int i,j;
		for(i = 0; i < 6; i++){
			sempool[i].P();
			if(pool[i]){
				pool[i] = false;
				printf("%s consumes %d |",currentThread->getName(),i);
				for(j = 0; j < 6; j++)
					printf(" %d",pool[j]);
				printf("\n");
				sempool[i].V();
				currentThread->Yield();
				break;

			}
			else{
				sempool[i].V();
			}
		}
		semNempty.V();
	}
}
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
PrintReadyList()
{
    IntStatus oldlevel = interrupt->SetLevel(IntOff);
    printf("thread name %s tid_ %d status: %s\n",currentThread->getName(),currentThread->get_tid_(),ThreadStatusChar[currentThread->getStatus()]);
    List *list = new List();
    list = scheduler->getReadyList();
    if(!list->IsEmpty()){
	list->Mapcar(MyThreadPrint);
    }
    currentThread->Yield();
    (void*)interrupt->SetLevel(oldlevel);
}



void
SimpleThread(int which)
{
    int num;
	for(num = 0; num < 100; num++){
		interrupt->SetLevel(IntOn);
	//	printf("thread %d name %s priority %d looped %d times\n",which,currentThread->getName(),currentThread->get_pp(),num);
		interrupt->SetLevel(IntOff);
	}
/*
	int temp;
	switch(currentThread->get_pp()){
		case 0:temp = TimerTicks;break;
		case 1:temp = TimerTicks1;break;
		case 2:temp = TimerTicks2;break;
		default:temp = TimerTicks3;break;
	}
if(stats->systemTicks - lastSwitchTime < temp ){
	int nnum = (temp -10*(temp/TimerTicks)- (stats->systemTicks - lastSwitchTime))/10;
	for(int i = 0; i < nnum; i++){
		printf("thread name %s happy to get extra time %d\n",currentThread->getName(),i);
		interrupt->SetLevel(IntOn);
		interrupt->SetLevel(IntOff);
	}
    }
*/
 //foooooor lab2 ex3---------------------------------------------   
/*    for (num = 0; num < 5; num++) {
	printf("*** thread %d  name %s uid_ %d tid_ %d priority %d looped %d times\n Status %s\n", which,currentThread->getName(),currentThread->get_uid_(),currentThread->get_tid_(), currentThread->get_pp(),num,ThreadStatusChar[currentThread->getStatus()]);
    if(num == 0){
	printf("reached here.\n");  
        currentThread->Yield();
    }
 }*/
//-----------------------------------------------------


  
   // printf("Status %s\n",currnentThread->getStatus());
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t1 = new Thread("forked thread1",0);
    Thread *t2 = new Thread("forked thread2",0);
    Thread *t3 = new Thread("forked thread3",0);

    t1->Fork(SimpleThread,/*t1->get_tid_()*/1);
	
    t2->Fork(SimpleThread,/*t2->get_tid_()*/2);
	
    t3->Fork(SimpleThread,/*t3->get_tid_()*/3);
/*
	int temp;
	switch(currentThread->get_pp()){
		case 0:temp = TimerTicks;break;
		case 1:temp = TimerTicks1;break;
		case 2:temp = TimerTicks2;break;
		default:temp = TimerTicks3;break;
	}
    if((stats->systemTicks -lastSwitchTime) % temp < temp){
	int num = (temp - ((stats->systemTicks - lastSwitchTime) % temp))/10;
	for(int i = 0; i < num; i++){
		printf("main happy to get extra time %d\n",i);
		interrupt->SetLevel(IntOn);
		interrupt->SetLevel(IntOff);
	}
    }
*/
   // currentThread->set_pp(7);
    
//    SimpleThread(0);
}
//---------------------------------lab2
void
mytest12(){
    for(int num = 0; num < 3; num++)
	printf("*** thread name %s priority %d looped %d times.\n",currentThread->getName(),currentThread->get_pp(),num);
}
void
mytest1(){
    DEBUG('t',"Entering mytest1");
    Thread *t1 = new Thread("thread1",6);
    for(int num = 0; num < 3; num++){
        if(num == 0){
	    Thread *t2 = new Thread("thread2",3);
	    t1->Fork(mytest12,t1->get_tid_());
	}
	printf("*** thread   name %s  priority %d looped %d times\n Status %s\n", currentThread->getName(), currentThread->get_pp(),num,ThreadStatusChar[currentThread->getStatus()]);
    }
}
//----------------------------------
void
ThreadTest3()
{
    DEBUG('t',"Entering ThreadTest3");
    
    Thread *t1 = new Thread("forked thread1");
    Thread *t2 = new Thread("forked thread2");
    Thread *t3 = new Thread("forked thread3");

    t1->Fork(PrintReadyList,0);
    t2->Fork(PrintReadyList,0);
    t3->Fork(PrintReadyList,0);

}

void
ThreadTest33()
{
	Thread *producer[5],*consumer[5];
	producer[0] = new Thread("Producer 0",1);
	producer[1] = new Thread("Producer 1",1);
	producer[2] = new Thread("Producer 2",1);
	producer[3] = new Thread("Producer 3",1);
	producer[4] = new Thread("Producer 4",1);
	consumer[0] = new Thread("Consumer 0",1);
	consumer[1] = new Thread("Consumer 1",1);
	consumer[2] = new Thread("Consumer 2",1);
	
	for(int i = 0; i < 5; i++){
		producer[i]->Fork(Produce,0);
	}	
	for(int i = 0; i < 3; i++){
		consumer[i]->Fork(Consume,0);
	}
}
void
ThreadTest32()
{
	proList = new List;
	lock = new Lock("lock");

	Thread *t1 = new Thread("producer1");
	Thread *t2 = new Thread("consumer1");
	Thread *t3 = new Thread("consumer2");
	Thread *t4 = new Thread("producer2");
	t1->Fork(produce2,1);
	t2->Fork(consume2,1);
	t3->Fork(consume2,1);
	t4->Fork(produce2,1);
}
void ThreadTest31()
{
	lock = new Lock("mylock");
	condition = new Condition("myCondition");
	Thread *t1 = new Thread("t1");
	Thread *t2 = new Thread("t2");
	Thread *t3 = new Thread("t3");
	
	t1->Fork(barrierTest,1);
	t2->Fork(barrierTest,1);
	t3->Fork(barrierTest,1);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    case 3:
	ThreadTest3();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

