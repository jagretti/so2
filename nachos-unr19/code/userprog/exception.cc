/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2019 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "args.hh"

//---------------------------------------------------------------------
// Funciones que leen y escriben en el espacio de usuario
//---------------------------------------------------------------------
/*void
ReadStringFromUser(int usrAddr, char *outStr, unsigned byteCount)
{
    int i = 0, c;
    if (byteCount < 1) return;
    do {
        ASSERT(machine->ReadMem(usrAddr+i,1,&c));
        outStr[i] = c;
    } while (outStr[i++] != '\0' && i < byteCount);
}

void
ReadBufferFromUser(int usrAddr, char *outBuff, int byteCount)
{
    int c, i = 0;
    while (byteCount > 0) {
        ASSERT(machine->ReadMem(usrAddr+i,1,&c));
        outBuff[i++] = c;
        byteCount--;
    }
}

void
WriteStringToUser(char *str, int usrAddr)
{
    int c, i = 0;
    while ((c = str[i]) != '\0') {
        ASSERT(machine->WriteMem(usrAddr+i,1,c));
        i++;
    }
}

void
WriteBufferToUser(char *str, int usrAddr, int byteCount)
{
    int i = 0;
    while (byteCount > 0) {
        ASSERT(machine -> WriteMem(usrAddr+i,1,str[i]));
        byteCount--;
        i++;
    }
}
*/

//---------------------------------------------------------------------
// Funciones que trabajan sobre la procTable
//---------------------------------------------------------------------
SpaceId
getNextId(Thread *t)
{
    for(int i = 0; i < MAX_PROCESS; i++) {
        if (procTable[i] == NULL) {
            procTable[i] = t;
            return i;
        }
    }
    return -1;
}

void
freeId(SpaceId id)
{
    if (id < MAX_PROCESS and id > 0) {
        procTable[id] = NULL;
    }
}

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

static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

    case SC_HALT: {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
        IncrementPC();
        break;
    }

    case SC_CREATE: {
        int filenameAddr = machine->ReadRegister(4);
        if (filenameAddr == 0)
            DEBUG('a', "Error: address to filename string is null.\n");

        char filename[FILE_NAME_MAX_LEN + 1];
        if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
            DEBUG('a', "Error: filename string too long (maximum is %u bytes).\n",
                  FILE_NAME_MAX_LEN);

        DEBUG('a', "Open requested for file `%s`.\n", filename);
        if (fileSystem->Create(filename,0)) {
            DEBUG('a', "Se creo el archivo %s correctamente\n", filename);
            machine->WriteRegister(2, 1);
        }
        else {
            DEBUG('a', "Error creando el archivo %s \n", filename);
            machine->WriteRegister(2, -1);
        }
        IncrementPC();
        break;
    }

    case SC_OPEN: {
        char *name = new char[128];
        ReadStringFromUser(machine->ReadRegister(4),name, 128);
        OpenFile *f = fileSystem->Open(name);
        if (f == NULL) {
            DEBUG('a', "Archivo con nombre %s vacio\n", name);
            machine->WriteRegister(2, -1);
        } else {
            int fd = currentThread->AddFile(f);
            machine->WriteRegister(2, fd);
            DEBUG('a', "Se abrio archivo con nombre %s y fd %d\n", name, fd);
        }
        IncrementPC();
        delete []name;
        break;
    }

    case SC_CLOSE: {
        int fid = machine->ReadRegister(4);
        DEBUG('a', "Close requested for id %u.\n", fid);
        if (fid < 2) {
            DEBUG('a', "Hubo un error cerrando un archivo");
            machine->WriteRegister(2, -1);
        } else {
            currentThread->CloseFile(fid);
            DEBUG('a', "Se cerro archivo con fd %d", fid);
        }
        IncrementPC();
        break;
    }

    case SC_WRITE: {
        int fd = machine->ReadRegister(6);
        int size = machine->ReadRegister(5);
        char *buff = new char[size];
        int write = -1;
        if (fd == 0) { // Error - no se puede escribir en stdin
            machine->WriteRegister(2, write);
            delete []buff;
            break;
        } else {
            if (fd == 1) { // Escribe en stdout
                ReadBufferFromUser(machine->ReadRegister(4), buff, size);
                for(int i = 0; i < size; i++) {
                    sconsole->WriteChar(buff[i]);
                }
                write = size - 1;
            } else {
                OpenFile *f = currentThread->GetFile(fd);
                if (f == NULL) {
                    DEBUG('a', "El archivo %d no esta abierto", fd);
                    machine->WriteRegister(2, write);
                    delete []buff;
                    break;
                }
                // Leo del espacio de usuario el string a escribir
                int arg = machine->ReadRegister(4);
                ReadStringFromUser(arg, buff, size);
                size = strlen(buff);
                DEBUG('a', "Escribo en archivo %d\n", fd);
                write = f->Write((const char*)buff, size);
            }
        }
        machine->WriteRegister(2, write);
        IncrementPC();
        delete []buff;
        break;
    }

    case SC_READ:{
        int fd = machine->ReadRegister(6);
        int size = machine->ReadRegister(5);
        char *buff = new char[size];
        int read = -1;
        if (fd == 1) { //stdout
            machine->WriteRegister(2, read);
            delete []buff;
            break;
        } else {
            if (fd == 0) { //stdin
                char c;
                for(int i = 0; i < size; i++) {
                    c = sconsole->ReadChar();
                    buff[i] = c;
                }
                WriteBufferToUser(buff, machine->ReadRegister(4), size);
                read = size - 1;
            } else {
                OpenFile *f = currentThread->GetFile(fd);
                if (f == NULL) {
                    machine->WriteRegister(2, read);
                    delete []buff;
                    break;
                }
                read = f->Read(buff, size);
                WriteBufferToUser(buff, machine->ReadRegister(4),read);
            }
        }
        machine->WriteRegister(2,read);
        DEBUG('a', "Leo en archivo %d\n", fd);
        IncrementPC();
        delete []buff;
        break;
    }

    case SC_EXIT:{
        int s = machine->ReadRegister(4);
        currentThread->exitStatus = s;
        currentThread->Finish();
        IncrementPC();
        break;
    }
    case SC_JOIN:{
        int id = machine->ReadRegister(4);
        Thread *t = procTable[id];
        t->Join();
        machine->WriteRegister(2, 0);
        IncrementPC();
        break;
    }
    case SC_EXEC:{
        char *path = new char[128];
        int name_addr = machine->ReadRegister(4);
        int args_addr = machine->ReadRegister(5);
        ReadStringFromUser(name_addr, path, 128);
        OpenFile *executable = fileSystem->Open(path);
        if (executable == NULL) {
            delete []path;
            break;
        }
        Thread *t = new Thread(path, 0, true);
        AddressSpace *addr = new AddressSpace(executable);
        if (!addr->IsValid()) { // El espacio de direcciones no es valido!
            t->exitStatus = 0;
            t->Finish();
            delete addr;
            delete []path;
            break;
        }
        t->space = addr;
        int id = getNextId(t);
        char **args = SaveArgs(args_addr);
        // imprimo todos los argumentos a escribir;
        //int i = 0;
        //while(args[i] != NULL) printf("args > %s \n",args[i++]);
        //
        t->Fork(beginProcess, args);
        machine->WriteRegister(2, id);
        IncrementPC();
        delete []path;
        break;
    }
    default:
        fprintf(stderr, "Unexpected system call: id %d.\n", scid);
        ASSERT(false);

    }

    IncrementPC();
}


/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
