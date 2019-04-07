// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "args.cc"

//Ejercicio 1

//---------------------------------------------------------------------
// Funciones que leen y escriben en el espacio de usuario
//---------------------------------------------------------------------
void readStrFromUser(int usrAddr, char *outStr, unsigned byteCount)
{
    int i = 0, c;
    if (byteCount < 1) return;
    do {
        //ASSERT(machine->ReadMem(usrAddr+i,1,&c));
        bool read = machine->ReadMem(usrAddr+i,1,&c);
        DEBUG('a', "Lectura ReadMem %d\n", read);
        if(read) {
            outStr[i] = c;
        } else {
            read = machine->ReadMem(usrAddr+i,1,&c);
            outStr[i] = c;
        }
    } while (outStr[i++] != '\0' && i < byteCount);
}

void readBuffFromUsr(int usrAddr, char *outBuff, int byteCount) 
{
    int c, i = 0;
    while (byteCount > 0) {
        //ASSERT(machine->ReadMem(usrAddr+i,1,&c));
        bool read = machine->ReadMem(usrAddr+i,1,&c);
        if(read) {
            outBuff[i] = c;
        } else {
            read = machine->ReadMem(usrAddr+i,1,&c);
            outBuff[i] = c;
        }
        //outBuff[i++] = c;
        i++;
        byteCount--;
    }
}

void writeStrToUsr(char *str, int usrAddr) 
{
    int c, i = 0;
    while ((c = str[i]) != '\0') {
        //ASSERT(machine->WriteMem(usrAddr+i,1,c));
        bool write = machine->WriteMem(usrAddr+i,1,c);
        if(!write) {
            machine->WriteMem(usrAddr+i,1,c);   
        }
        i++;
    }
}

void writeBuffToUsr(char *str, int usrAddr, int byteCount)
{
    int i = 0;
    while (byteCount > 0) {
        //ASSERT(machine -> WriteMem(usrAddr+i,1,str[i]));
        bool write = machine -> WriteMem(usrAddr+i,1,str[i]);
        if(!write) {
            machine -> WriteMem(usrAddr+i,1,str[i]);
        }
        byteCount--;
        i++;
    }
}
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Funcion que incrementa el Program Counter
//---------------------------------------------------------------------
void
incrementarPC() 
{
    int pc = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg,pc);
    pc = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg,pc);
    pc += 4;
    machine->WriteRegister(NextPCReg,pc);
}
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Funciones que trabajan sobre la procTable
//---------------------------------------------------------------------
SpaceId getNextId(Thread *t) 
{
    for(int i = 0; i < MAX_PROC; i++) {
        if (procTable[i] == NULL) {
            procTable[i] = t;
            return i;
        }
    }        
    return -1;
}

void freeId(SpaceId id)
{
    if (id < MAX_PROC and id > 0) {
        procTable[id] = NULL;
    }
}
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Funcion a forkear en un nuevo thread, inicia los registros en el espacio
// de direcciones y escribe los argumentos de lo que se ejecuta
//---------------------------------------------------------------------
void beginProcess(void *args)
{
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    WriteArgs((char **) args);
    machine->Run();
}
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Guarda en la TLB la referencia a esa entrada de la pageTable
//---------------------------------------------------------------------
int SaveInTLB(TranslationEntry toSave, int position)
{
    DEBUG('f', "Guardo en la tlb position %d\n", position);
    TranslationEntry entry = machine->tlb[position];
    if(entry.valid) {
        if(entry.dirty) {
            currentThread->space->SaveEntry(entry);
        }
    }
    machine->tlb[position] = toSave;
    return 1;
}

void printTLB()
{
    for(int i = 0; i < TLBSize; i++) {
        printf("TLB[%d] -- ", i);
        printf("physical: %d virtual: %d dirty: %d valid: %d\n", machine->tlb[i].physicalPage, machine->tlb[i].virtualPage, machine->tlb[i].dirty, machine->tlb[i].valid);
    }
}

int SaveInTLB_old(TranslationEntry toSave)
{
    int position = -1;
    int i = 0;
    for(i = 0; i < TLBSize; i++) {
        if(!machine->tlb[i].valid) {
            position = i;
            break;
        }
    }   
    if(position < 0) {
        TranslationEntry toMem = machine->tlb[TLBSize - 1];
        if(toMem.dirty) {
            currentThread->space->SaveEntry(toMem);
        }
        for(i = 1; i < TLBSize; i++) {
            machine->tlb[i] = machine->tlb[i-1];
        }    
        position = 0;
    }
    machine->tlb[position] = toSave;
    return 1;
}
//---------------------------------------------------------------------

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        switch (type) {
            case SC_Halt:{
	            DEBUG('a', "Shutdown, initiated by user program.\n");
   	            interrupt->Halt();
            }
            case SC_Create:{
                int arg = machine->ReadRegister(4);
                char *name = new char[128];
                readStrFromUser(arg, name, 128);
                if (fileSystem->Create(name,0)) {
                    DEBUG('a', "Se creo el archivo %s correctamente\n", name);
                    machine->WriteRegister(2, 1);
                }
                else {
                    DEBUG('a', "Error creando el archivo %s \n", name);
                    machine->WriteRegister(2, -1);
                }
                delete []name;
                incrementarPC();
                break;
            }
            case SC_Write:{
                int fd = machine->ReadRegister(6);
                int size = machine->ReadRegister(5);
                char *buff = new char[size];
                int write = -1;
                if (fd == 0) { // Error - no se puede escribir en stdin
                    machine->WriteRegister(2, write);
                    incrementarPC();
                    delete []buff;
                    break;
                } else {
                    if (fd == 1) { // Escribe en stdout
                        readBuffFromUsr(machine->ReadRegister(4), buff, size);
                        for(int i = 0; i < size; i++) {
                            sconsole->WriteChar(buff[i]);
                        }
                        write = size - 1;
                    } else {
                        OpenFile *f = currentThread->GetFile(fd);
                        if (f == NULL) {
                            DEBUG('a', "El archivo %d no esta abierto", fd);
                            machine->WriteRegister(2, write);
                            incrementarPC();
                            delete []buff;
					        break;
                        }
                        // Leo del espacio de usuario el string a escribir
                        int arg = machine->ReadRegister(4);
                        readStrFromUser(arg, buff, size);
                        size = strlen(buff);
                        DEBUG('a', "Escribo en archivo %d\n", fd);
                        write = f->Write((const char*)buff, size);
                    }
                }
                machine->WriteRegister(2, write);
                delete []buff;
                incrementarPC();
                break;
            }
            case SC_Read:{
				int fd = machine->ReadRegister(6);
				int size = machine->ReadRegister(5);
				char *buff = new char[size];
                int read = -1;
                if (fd == 1) { //stdout
                    machine->WriteRegister(2, read);
                    incrementarPC();
                    delete []buff;
                    break;
                } else {
                    if (fd == 0) { //stdin
                        char c;
                        for(int i = 0; i < size; i++) {
                            c = sconsole->ReadChar();
                            buff[i] = c;
                        }
                        DEBUG('k', "%c\n", buff[0]);
				        writeBuffToUsr(buff, machine->ReadRegister(4), size);
                        read = size - 1;
                    } else {
				        OpenFile *f = currentThread->GetFile(fd);
				        if (f == NULL) {
                            machine->WriteRegister(2, read);
					        incrementarPC();
                            delete []buff;
					        break;
				        }
				        read = f->Read(buff, size);
				        writeBuffToUsr(buff, machine->ReadRegister(4),read);
                    }
                }
				machine->WriteRegister(2,read);
                DEBUG('a', "Leo en archivo %d\n", fd);
				incrementarPC();
                delete []buff;
                break;
			}
            case SC_Open:{
				char *name = new char[128];
				readStrFromUser(machine->ReadRegister(4),name, 128);
				OpenFile *f = fileSystem->Open(name);
				if (f == NULL) {
                    DEBUG('a', "Archivo con nombre %s vacio\n", name);
					machine->WriteRegister(2, -1);
				} else {
					int fd = currentThread->AddFile(f);
					machine->WriteRegister(2, fd);
                    DEBUG('a', "Se abrio archivo con nombre %s y fd %d\n", name, fd);
				}
				incrementarPC();
                delete []name;
                break;
			}
            case SC_Close:{
				int fd = machine->ReadRegister(4);
                if (fd == NULL) {
                    DEBUG('a', "Hubo un error cerrando un archivo");
                    machine->WriteRegister(2, -1);
                } else {
				    currentThread->CloseFile(fd);
                    DEBUG('a', "Se cerro archivo con fd %d", fd);
                }
				incrementarPC();
                break;
			}
            case SC_Exit:{
				int s = machine->ReadRegister(4);
				currentThread->exitS = s;
				currentThread->Finish();
				break;
            }
            case SC_Join:{
                int id = machine->ReadRegister(4);
                id--;
                Thread *t = procTable[id];
                t->Join();
                machine->WriteRegister(2, 0);
                incrementarPC();
                break;
            }
            case SC_Exec:{
				char *path = new char[128];
                int name_addr = machine->ReadRegister(4);
                int args_addr = machine->ReadRegister(5);
				readStrFromUser(name_addr, path, 128);
                DEBUG('a', "Corriendo programa %s\n", path);
                OpenFile *executable = fileSystem->Open(path);
				if (executable == NULL) {
                    DEBUG('a', "No se pudo abrir/encontrar el ejecutable %s", path);
                    delete []path;
					break;
                }
                Thread *t = new Thread(path, 0, true);
				AddrSpace *addr = new AddrSpace(executable);
                if (!addr->IsValid()) { // El espacio de direcciones no es valido!
                    DEBUG('a', "El espacio de direcciones no es valido!\n");
                    t->exitS = 0;
                    t->Finish();
                    delete addr;
                    delete []path;
                    break;
                }
                t->space = addr;
                int id = getNextId(t);
                char **args = SaveArgs(args_addr);
                t->Fork(beginProcess, args);
                machine->WriteRegister(2, id);
                incrementarPC();
                delete []path;
                break;
            }
        }
    } else if(which == PageFaultException) {
        static int position = 0;
        int virtualAddr = machine->ReadRegister(BadVAddrReg);
        int virtualPage = virtualAddr / PageSize;
        int offset = virtualAddr % PageSize;
        DEBUG('f', "PageFault de pagina %d\n", virtualPage); 
        // Busco la entrada en el espacio de direcciones del thread actual
        TranslationEntry entry = currentThread->space->GetEntry(virtualPage);
        SaveInTLB(entry, position);
        position = (position + 1) % TLBSize;
        printTLB();
        machine->WriteRegister(2, 1); 
    } else {
	    printf("Unexpected user mode exception %d %d\n", which, type);
	    ASSERT(false);
    }
}


