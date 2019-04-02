// synchconsole.h
// Rutinas para acceder de manera sincrona a la consola
// La consola es un dispositivo asincrono, esta clase
// provee una interfaz de acceso sincrono
// José Francisco Agretti

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "machine/console.hh"
#include "synch.hh"

// La clase provee una abstraccion de
// accesso sincronizado a la consola
class SynchConsole {
public:
    SynchConsole(const char* readFile, const char* writeFile);

    ~SynchConsole();

    void WriteChar(char data);

    char ReadChar();

    void ConsoleWriteDone(void*);

    void ConsoleReadAvailable(void*);

private:
    Console *console;
    Semaphore *readSemaphore;
    Semaphore *writeSemaphore;
    Lock *readLock;
    Lock *writeLock;

};

#endif //SYNCHCONSOLE_H
