// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create several threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//
// Parts from Copyright (c) 2007-2009 Universidad de Las Palmas de Gran Canaria
//

#include "copyright.h"
#include "system.h"


#ifdef SEMAPHORE_TEST
    #include "synch.h"
    Semaphore* sem = new Semaphore("threadtest",3);
#endif //SEMAPHORE_TEST
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//      debugging purposes.
//----------------------------------------------------------------------

void
SimpleThread(void* name)
{
    // Reinterpret arg "name" as a string
    char* threadName = (char*)name;
    
    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.
    for (int num = 0; num < 10; num++) {
        #ifdef SEMAPHORE_TEST
        DEBUG('t',"%s hace un P()\n",threadName);
        sem->P();
        #endif

        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	    printf("*** thread %s looped %d times\n", threadName, num);
	    //interrupt->SetLevel(oldLevel);
        currentThread->Yield();

        #ifdef SEMAPHORE_TEST
        DEBUG('t',"%s hace un V()\n",threadName);
        sem->V();
        #endif
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> Thread %s has finished\n", threadName);
    //interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several threads, by launching
//	ten threads which call SimpleThread, and finally calling 
//	SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");
    int numthreads = 5;
    for(int i = 1; i < numthreads; i++) {
    	char *threadname = new char[128];
    	sprintf(threadname, "Hilo %d",i);
        Thread* newThread = new Thread (threadname);
    	newThread->Fork (SimpleThread, (void*)threadname);
   	}
    
    SimpleThread( (void*)"Hilo 0");
}

