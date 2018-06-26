// synchconsole.cc
// Implementacion de la interfaz de acceso
// sincrono a la consola

// José Francisco Agretti

#include "synchconsole.h"

//----------------------------------------------------------------------
// callback de WriteDone
//----------------------------------------------------------------------
static void ConsoleWriteDone(void* arg)
{
    SynchConsole* console = (SynchConsole*) arg;

    console->writeSemaphore->V();
}

//----------------------------------------------------------------------
// callback de ReadAvailable
//----------------------------------------------------------------------
static void ConsoleReadAvailable(void *arg)
{
    SynchConsole* console = (SynchConsole*) arg;

    console->readSemaphore->V();
}

//----------------------------------------------------------------------
//Constructor
//----------------------------------------------------------------------
SynchConsole::SynchConsole(const char* read_file, const char* write_file)
{
    console = new Console(read_file, write_file, ConsoleReadAvailable,
                    ConsoleWriteDone, this);
    readLock = new Lock("SynchConsole Read Lock");
    writeLock = new Lock("SynchConsole Write Lock");
    readSemaphore = new Semaphore("SynchConsole Read Semaphore", 0);
    writeSemaphore = new Semaphore("SynchConsole Write Semaphore", 0);
}

//----------------------------------------------------------------------
// WriteChar
// Lockeo al escribir, y veo que se termine de escribir para
// terminar
//----------------------------------------------------------------------
void WriteChar(char c):
{
    writeLock->Acquire();
    console->PutChar(c);
    writeSemaphore->P();
    writeLock->Release();
}

//----------------------------------------------------------------------
// ReadChar
// Veo que se termine de leer para retornar
//----------------------------------------------------------------------
char ReadChar()
{
    readLock->Acquire();
    char c = console->GetChar();
    readSemaphore->P();
    readLock->Release();
    return c;
}

