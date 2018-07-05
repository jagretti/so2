// synchconsole.cc
// Implementacion de la interfaz de acceso
// sincrono a la consola

// José Francisco Agretti

#include "synchconsole.h"

//----------------------------------------------------------------------
// Funciones que llaman a los metodos internos de la clase
//----------------------------------------------------------------------
static void CallConsoleReadAvailable(void *arg)
{
    SynchConsole* console = (SynchConsole*) arg;

    console->ConsoleReadAvailable(console);
}

static void CallConsoleWriteDone(void* arg)
{
    SynchConsole* console = (SynchConsole*) arg;

    console->ConsoleWriteDone(console);
}
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//Constructor
//----------------------------------------------------------------------
SynchConsole::SynchConsole(const char* read_file, const char* write_file)
{
    readLock = new Lock("SynchConsole Read Lock");
    writeLock = new Lock("SynchConsole Write Lock");
    readSemaphore = new Semaphore("SynchConsole Read Semaphore", 0);
    writeSemaphore = new Semaphore("SynchConsole Write Semaphore", 0);
    console = new Console(read_file, write_file, CallConsoleReadAvailable, CallConsoleWriteDone, NULL);
}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
SynchConsole::~SynchConsole()
{
    delete console;
    delete readLock;
    delete readSemaphore;
    delete writeSemaphore;
}


//----------------------------------------------------------------------
// WriteChar
// Lockeo al escribir, y veo que se termine de escribir para
// terminar
//----------------------------------------------------------------------
void SynchConsole::WriteChar(char c)
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
char SynchConsole::ReadChar()
{
    readLock->Acquire();
    char c = console->GetChar();
    readSemaphore->P();
    readLock->Release();
    return c;
}

//----------------------------------------------------------------------
// callback de WriteDone
//----------------------------------------------------------------------
void SynchConsole::ConsoleWriteDone(void* arg)
{
    SynchConsole* sconsole = (SynchConsole*) arg;

    sconsole->writeSemaphore->V();
}

//----------------------------------------------------------------------
// callback de ReadAvailable
//----------------------------------------------------------------------
void SynchConsole::ConsoleReadAvailable(void *arg)
{
    SynchConsole* sconsole = (SynchConsole*) arg;

    sconsole->readSemaphore->V();
}


