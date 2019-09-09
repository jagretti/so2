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
// Funcion a forkear en un nuevo thread, inicia los registros en el espacio
// de direcciones y escribe los argumentos de lo que se ejecuta
//---------------------------------------------------------------------
void beginProcess(void *args)
{
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    // Escribo argc en el registro 4
    int argc = WriteArgs((char **) args);
    machine->WriteRegister(4, argc);
    // Escribo argv en el registro 5 (sumo 16 por que WriteArgs antes de retornar los resta!
    int argv = (machine->ReadRegister(STACK_REG)) + 16;
    machine->WriteRegister(5, argv);
    machine->Run();
}

//---------------------------------------------------------------------
// Guarda en la TLB la referencia a esa entrada de la pageTable
//---------------------------------------------------------------------
int SaveInTLB(TranslationEntry toSave, int position)
{
    // DEBUG('k', "Exception::SaveInTLB position %d\n", position);
    TranslationEntry entry = machine->GetMMU()->tlb[position];
    if(entry.valid && entry.dirty) {
        currentThread->space->SaveEntry(entry);
    }
    machine->GetMMU()->tlb[position] = toSave;
    return 1;
}

void printTLB()
{
    printf("--------------\n");
    for(unsigned i = 0; i < TLB_SIZE; i++) {
        printf("TLB[%d] -- ", i);
        printf("physical: %d virtual: %d dirty: %d valid: %d\n",
               machine->GetMMU()->tlb[i].physicalPage,
               machine->GetMMU()->tlb[i].virtualPage,
               machine->GetMMU()->tlb[i].dirty,
               machine->GetMMU()->tlb[i].valid);
    }
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
        DEBUG('k', "SC_HALT - Shutdown, initiated by user program.\n");
        interrupt->Halt();
        break;
    }

    case SC_CREATE: {
        int filenameAddr = machine->ReadRegister(4);
        if (filenameAddr == 0)
            DEBUG('k', "SC_CREATE - Error: address to filename string is null.\n");

        char *filename = new char[FILE_NAME_MAX_LEN + 1];
        if (!ReadStringFromUser(filenameAddr, filename, sizeof filename))
            DEBUG('k', "SC_CREATE - Error: filename string too long (maximum is %u bytes).\n",
                  FILE_NAME_MAX_LEN);

        DEBUG('k', "SC_CREATE - Open requested for file `%s`.\n", filename);
        if (fileSystem->Create(filename,0)) {
            DEBUG('k', "SC_CREATE - Se creo el archivo %s correctamente\n", filename);
        }
        else {
            DEBUG('k', "SC_CREATE - Error creando el archivo %s \n", filename);
        }
        delete []filename;
        break;
    }

    case SC_REMOVE: {
        int fileNameAddr = machine->ReadRegister(4);
        if (fileNameAddr == 0)
            DEBUG('a', "SC_REMOVE - Error: address to filename string is null\n");
        char *filename = new char[FILE_NAME_MAX_LEN + 1];
        ReadStringFromUser(fileNameAddr, filename, FILE_NAME_MAX_LEN);
        bool remove_ok = fileSystem->Remove(filename);
        if (!remove_ok) {
            DEBUG('k', "SC_REMOVE - Error borrando el archivo %s\n", filename);
            machine->WriteRegister(2, -1);
        } else {
            DEBUG('k', "SC_REMOVE - Se borro el archivo %s correctamente\n", filename);
        }
        delete []filename;
        break;
    }

    case SC_OPEN: {
        char *name = new char[128];
        ReadStringFromUser(machine->ReadRegister(4),name, 128);
        OpenFile *f = fileSystem->Open(name);
        if (f == nullptr) {
            DEBUG('k', "SC_OPEN - Archivo con nombre %s vacio\n", name);
            machine->WriteRegister(2, -1);
        } else {
            int fd = currentThread->AddFile(f);
            machine->WriteRegister(2, fd);
            DEBUG('k', "SC_OPEN - Se abrio archivo con nombre %s y fd %d\n", name, fd);
        }
        delete []name;
        break;
    }

    case SC_CLOSE: {
        int fd = machine->ReadRegister(4);
        DEBUG('k', "SC_CLOSE - close requested for id %u.\n", fd);
        if (fd < 2) {
            DEBUG('k', "SC_CLOSE - Hubo un error cerrando un archivo\n");
        } else {
            currentThread->CloseFile(fd);
            DEBUG('k', "SC_CLOSE - Se cerro archivo con fd %d\n", fd);
        }
        break;
    }

    case SC_WRITE: {
        int fd = machine->ReadRegister(6);
        int size = machine->ReadRegister(5);
        DEBUG('k', "SC_WRITE - fd = %d size = %d\n", fd, size);
        char *buff = new char[size];
        if (fd != 0) { // Error (fd == 0) no se puede escribir en stdin
            int arg = machine->ReadRegister(4);
            if (fd == 1) { // Escribe en stdout
                ReadStringFromUser(arg, buff, size);
                DEBUG('k', "SC_WRITE - escribir en la consola: |%s|\n", buff);
                for(int i = 0; i < size; i++) {
                    sconsole->WriteChar(buff[i]);
                }
            } else {
                OpenFile *f = currentThread->GetFile(fd);
                if (f == nullptr) {
                    DEBUG('k', "SC_WRITE - el archivo %d no esta abierto\n", fd);
                    delete []buff;
                    break;
                }
                // Leo del espacio de usuario el string a escribir
                ReadStringFromUser(arg, buff, size);
                size = strlen(buff);
                DEBUG('k', "SC_WRITE - escribo en archivo %d - chars %d |%s|\n", fd, size, buff);
                f->Write((const char*)buff, size);
            }
        }
        delete []buff;
        break;
    }

    case SC_READ:{
        int fd = machine->ReadRegister(6);
        int size = machine->ReadRegister(5);
        char *buff = new char[size];
        int read = -1;
        DEBUG('k', "SC_READ - fd = %d size = %d\n", fd, size);
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
                if (f == nullptr) {
                    machine->WriteRegister(2, read);
                    delete []buff;
                    break;
                }
                read = f->Read(buff, size);
                WriteBufferToUser(buff, machine->ReadRegister(4),read);
            }
        }
        machine->WriteRegister(2, read);
        delete []buff;
        break;
    }

    case SC_EXIT:{
        int s = machine->ReadRegister(4);
        DEBUG('k', "SC_EXIT - Status = %d nombre = %s\n", s, currentThread->GetName());
        currentThread->exitStatus = s;
        currentThread->Finish();
        break;
    }
    case SC_JOIN:{
        int id = machine->ReadRegister(4);
        Thread *t = procTable[id];
        DEBUG('k', "SC_JOIN - Id = %d nombre = %s\n", id, t->GetName());
        t->Join();
        machine->WriteRegister(2, t->exitStatus);
        break;
    }
    case SC_EXEC:{
        char *path = new char[128];
        int name_addr = machine->ReadRegister(4);
        int args_addr = machine->ReadRegister(5);
        DEBUG('k', "Direccion de path %d Direccion de args %d\n", name_addr, args_addr);
        ReadStringFromUser(name_addr, path, 128);
        OpenFile *executable = fileSystem->Open(path);
        if (executable == nullptr) {
            machine->WriteRegister(2, -1);
            delete []path;
            break;
        }
        Thread *t = new Thread(path, 0, true);
        AddressSpace *addr = new AddressSpace(executable);
        if (!addr->IsValid()) { // El espacio de direcciones no es valido!
            t->exitStatus = 0;
            t->Finish();
            machine->WriteRegister(2, -2);
            delete addr;
            delete []path;
            break;
        }
        t->space = addr;
        SpaceId id = getNextId(t);
        t->SetPid(id);
        char **args = SaveArgs(args_addr);
        t->Fork(beginProcess, args);
        DEBUG('k', "Ejecutar  %s args[1] %s\n", args[0], args[1]);
        machine->WriteRegister(2, id);
        //delete []path;
        break;
    }
    default:
        fprintf(stderr, "Unexpected system call: id %d.\n", scid);
        ASSERT(false);

    }
    IncrementPC();
}

static void
PageFaultHandler(ExceptionType et)
{
    // Posicion que va a ir la pagina en la tlb
    static int position = 0;
    int virtualAddr = machine->ReadRegister(BAD_VADDR_REG);
    int virtualPage = virtualAddr / PAGE_SIZE;
    DEBUG('k', "Exception::PageFaultHandler  virtualPage %d\n", virtualPage);
    // Busco la entrada en el espacio de direcciones del thread actual
    TranslationEntry entry = currentThread->space->GetEntry(virtualPage);
    #ifdef USE_DL
        //if (!entry.valid) {
        if (inFileOrSwap(entry.physicalPage)) {
            currentThread->space->LoadPage(virtualAddr);
            entry = currentThread->space->GetEntry(virtualPage);
        }
    #endif
    SaveInTLB(entry, position);
    // position varia entre 0,1,2,3 y asi sucesivamente haciendo un FIFO sobre la tlb
    position = (position + 1) % TLB_SIZE;
    //printTLB();
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
