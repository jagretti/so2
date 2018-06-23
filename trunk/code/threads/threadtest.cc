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
#include "synch.h"

#ifdef SEMAPHORE_TEST
    
    Semaphore* sem = new Semaphore("threadtest",3);
//    Lock* l = new Lock("lock_prueba");
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
//		l->Acquire();
        #endif

        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	    printf("*** thread %s looped %d times\n", threadName, num);
	    //interrupt->SetLevel(oldLevel);
        currentThread->Yield();

        #ifdef SEMAPHORE_TEST
        DEBUG('t',"%s hace un V()\n",threadName);
        sem->V();
//        l->Release();
        #endif
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> Thread %s has finished\n", threadName);
    //interrupt->SetLevel(oldLevel);
}

//Prueba de semaforos y locks

void SimpleThreadTest()
{
    DEBUG('t', "Entering SimpleTest");
    int numthreads = 5;
    for(int i = 1; i < numthreads; i++) {
    	char *threadname = new char[128];
    	sprintf(threadname, "Hilo %d",i);
        Thread* newThread = new Thread (threadname, NPRIO-1, false);
    	newThread->Fork (SimpleThread, (void*)threadname);
   	}
    
    SimpleThread( (void*)"Hilo 0");
}

//Prueba de Ports

void Productor(void *b)
{
    Port *p = (Port *)b;
    p->Send(5);
}

void Consumidor(void *b)
{
    int m;
    Port *p = (Port *)b;
    printf("Espero mensaje\n");
    p->Receive(&m);
    printf("Recibi el %d \n", m);
    p->Receive(&m);
    printf("Recibi el %d \n",m);
}

void PortTest()
{
    Port* p = new Port("puertoTest");
    Thread* newThread = new Thread ("Consumidores", NPRIO-1, false);
    printf("Lanzo Consumidores\n");
    newThread->Fork (Consumidor, (void*)p);

    newThread = new Thread ("Productor1", NPRIO-1, false);
    printf("Lanzo Productor1\n");
    newThread->Fork(Productor, (void*)p);

    printf("Lanzo Productor2\n");
    p->Send(2);
       
}

//Prueba de Join

void tHijo(void *a) 
{
    for(int i = 0; i < 1000000000; i++);
            
    printf("Termino hijo! \n");
}

void JoinTest()
{
    Thread* t = new Thread("Hijo", NPRIO-1, true);
    t->Fork(tHijo, NULL);
    printf("Forkee el hijo\n");
    printf("Hago Join\n");
    t->Join();
    printf("Termina padre! \n");
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
//Para probar descomentar cada funcion

//    SimpleThreadTest(); 
//    PortTest();
//    JoinTest();

}

