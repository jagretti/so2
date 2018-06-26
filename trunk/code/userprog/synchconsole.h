// synchconsole.h
// Rutinas para acceder de manera sincrona a la consola
// La consola es un dispositivo asincrono, esta clase
// provee una interfaz de accesi sincrono
// José Francisco Agretti

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "console.h"
#include "synch.h"

// La clase provee una abstraccion de
// accesso sincronizado a la consola

class SynchConsole {
    public:
        SynchConsole(const char *name);

        ~SynchConsole();

        void WriteChar(char *data);

        char ReadChar(char *data);

        //void WriteDone();

        //void ReadAvailable();

    private:
        Console *console;
        Semaphore *readSemaphore;
        Semaphore *writeSemaphore;
        Lock *readLock;    
        Lock *writeLock;    

}

#endif //SYNCHCONSOLE_H
