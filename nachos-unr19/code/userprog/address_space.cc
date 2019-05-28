/// Routines to manage address spaces (executing user programs).
///
/// In order to run a user program, you must:
///
/// 1. Link with the `-N -T 0` option.
/// 2. Run `coff2noff` to convert the object file to Nachos format (Nachos
///    object code format is essentially just a simpler version of the UNIX
///    executable object code format).
/// 3. Load the NOFF file into the Nachos file system (if you have not
///    implemented the file system yet, you do not need to do this last
///    step).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "bin/noff.h"
#include "machine/endianness.hh"
#include "threads/system.hh"


/// Do little endian to big endian conversion on the bytes in the object file
/// header, in case the file was generated on a little endian machine, and we
/// are re now running on a big endian machine.
static void
SwapHeader(noffHeader *noffH)
{
    ASSERT(noffH != nullptr);

    noffH->noffMagic              = WordToHost(noffH->noffMagic);
    noffH->code.size              = WordToHost(noffH->code.size);
    noffH->code.virtualAddr       = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr        = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size          = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr   = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr    = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size        = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr =
      WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr  = WordToHost(noffH->uninitData.inFileAddr);
}

/// Create an address space to run a user program.
///
/// Load the program from a file `executable`, and set everything up so that
/// we can start executing user instructions.
///
/// Assumes that the object code file is in NOFF format.
///
/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
///
/// * `executable` is the file containing the object code to load into
///   memory.
AddressSpace::AddressSpace(OpenFile *executable)
{
    ASSERT(executable != nullptr);

    // Comentamos esto, por que ahora noffH esta en AddressSpace
    // como propiedad private
    //noffHeader noffH;
    executable->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFF_MAGIC &&
          WordToHost(noffH.noffMagic) == NOFF_MAGIC)
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFF_MAGIC);

    // Guardamos el executable en el address_space
    address_exec = executable;

    // How big is address space?
    unsigned size = noffH.code.size + noffH.initData.size
                    + noffH.uninitData.size + USER_STACK_SIZE;
    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    // Check we are not trying to run anything too big -- at least until we
    // have virtual memory.
    //ASSERT(numPages <= NUM_PHYS_PAGES);

    // Seteo si es valido o no el address_space
    isValid = true; //numPages <= userProgramFrameTable->CountClear();
    // TODO
    char *str = new char[20];
    sprintf(str, "%d", size);
    const char *filename = str; //currentThread->GetName();
    fileSystem->Create(filename, 0);
    swapFile = fileSystem->Open(filename);
    currentThread->AddFile(swapFile);

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];
    //
    char *mainMemory = machine->GetMMU()->mainMemory;
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        #ifdef USE_DL
            pageTable[i].physicalPage = -1;
            pageTable[i].valid = false;
        #else
            pageTable[i].physicalPage = userProgramFrameTable->Find();
            pageTable[i].valid        = true;
            memset(&(mainMemory[pageTable[i].physicalPage * PAGE_SIZE]), 0, PAGE_SIZE);
        #endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
        // Zero out the physical address space
    }

    // Then, copy in the code and data segments into memory.
    #ifndef USE_DL
        for (unsigned i = 0; i < noffH.code.size; i++) {
            char byte;
            executable->ReadAt(&byte, 1, noffH.code.inFileAddr + i);
            int virtualAddr = noffH.code.virtualAddr + i;
            int virtualPageNum = virtualAddr / PAGE_SIZE;
            int offset = virtualAddr % PAGE_SIZE;
            int physicalPage = pageTable[virtualPageNum].physicalPage * PAGE_SIZE;
            mainMemory[physicalPage + offset] = byte;
        }

        for (unsigned i = 0; i < noffH.initData.size; i++) {
            char byte;
            executable->ReadAt(&byte, 1, noffH.initData.inFileAddr + i);
            int virtualAddr = noffH.initData.virtualAddr + i;
            int virtualPageNum = virtualAddr / PAGE_SIZE;
            int offset = virtualAddr % PAGE_SIZE;
            int physicalPage = pageTable[virtualPageNum].physicalPage * PAGE_SIZE;
            mainMemory[physicalPage + offset] = byte;
        }
    #endif
}

/// Deallocate an address space.
///
/// VER SI ES NECESARIO EL IF
AddressSpace::~AddressSpace()
{
    for (unsigned i = 0; i < numPages; i++) {
        // Si tiene asignada pagina fisica, la borro
        if (pageTable[i].physicalPage != -1) {
            //userProgramFrameTable->Clear(pageTable[i].physicalPage);
            //memset(&(machine->GetMMU()->mainMemory[pageTable[i].physicalPage * PAGE_SIZE]), 0, PAGE_SIZE);
        }
    }
    delete [] pageTable;
    // TODO currentThread->CloseFile(swapFile);
    fileSystem->Remove(currentThread->GetName());
    //

}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{
#ifdef USE_TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        if (machine->GetMMU()->tlb[i].dirty and machine->GetMMU()->tlb[i].valid) { //PREGUNTAR SI ESTA BIEN!!
            this->SaveEntry(machine->GetMMU()->tlb[i]);
        }
    }
    for (unsigned i = 0; i < numPages; i++) {
        if (pageTable[i].physicalPage != -1 || pageTable[i].physicalPage != -2) {
            UnloadPage(i);
        }
    }
#endif
}


/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
#ifdef USE_TLB
    // Limpio la tlb
    DEBUG('f', "Limpio TLB!\n");
    for(unsigned i = 0; i < TLB_SIZE; i++) {
        machine->GetMMU()->tlb[i].valid = false;
    }
#else
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
#endif

}

//----------------------------------------------------------------------
// Retorna si el espacio de direcciones es valido
//----------------------------------------------------------------------
bool
AddressSpace::IsValid()
{
    return this->isValid;
}

//----------------------------------------------------------------------
// Obtener la i-esima posicion de la pageTable asociada
// al espacio de direcciones del thread
//----------------------------------------------------------------------
TranslationEntry
AddressSpace::GetEntry(int virtualPageIndex)
{
    return this->pageTable[virtualPageIndex];
}

//----------------------------------------------------------------------
// Guarda la entrada toSave en la pageTable del proceso
//----------------------------------------------------------------------
void
AddressSpace::SaveEntry(TranslationEntry toSave)
{
    int vpn = toSave.virtualPage;
    this->pageTable[vpn] = toSave;
}

//-----------------------------------------------------------
// Lee del ejecutable y copia en memoria
//-----------------------------------------------------------
void copyVirtualAddressToMemory(unsigned virtualAddress, OpenFile *file, noffHeader noffH ,  char *memory, TranslationEntry *pageTable) {
    unsigned isCode = virtualAddress < noffH.code.size;
    if (isCode) {
        char byte;
        file->ReadAt(&byte, 1, noffH.code.inFileAddr + virtualAddress);
        int virtualAddr = virtualAddress;
        int virtualPageNum = virtualAddr / PAGE_SIZE;
        int offset = virtualAddr % PAGE_SIZE;
        int physicalPage = pageTable[virtualPageNum].physicalPage * PAGE_SIZE;
        memory[physicalPage + offset] = byte;
    }

    bool isInitData = virtualAddress >= noffH.initData.virtualAddr && virtualAddress < (noffH.initData.virtualAddr + noffH.initData.size);
    if (isInitData) {
        unsigned i = virtualAddress - noffH.initData.virtualAddr;
        char byte;
        file->ReadAt(&byte, 1, noffH.initData.inFileAddr + i);
        int virtualAddr = noffH.initData.virtualAddr + i;
        int virtualPageNum = virtualAddr / PAGE_SIZE;
        int offset = virtualAddr % PAGE_SIZE;
        int physicalPage = pageTable[virtualPageNum].physicalPage * PAGE_SIZE;
        memory[physicalPage + offset] = byte;
    }

}

//----------------------------------------------------------------------
// Carga la pagina donde se encuentra la virtualAddress
//----------------------------------------------------------------------
void
AddressSpace::LoadPage(unsigned virtualAddress)
{
    char *mainMemory = machine->GetMMU()->mainMemory;
    // Busco pagina virtual
    int virtualPage = virtualAddress / PAGE_SIZE;
    // Le asigno una fisica solo si no fue asignada antes
    if (pageTable[virtualPage].physicalPage == -1) {
        pageTable[virtualPage].physicalPage = memoryManager->AllocMemory(this, virtualPage);
        // first byte of the page
        unsigned address = (virtualAddress / PAGE_SIZE) * PAGE_SIZE;
        for (unsigned i = 0; i < PAGE_SIZE; i++) {
            copyVirtualAddressToMemory(address + i, address_exec, noffH, \
                                       mainMemory, pageTable);
        }
    }
    // La pagina esta en swap
    if (pageTable[virtualPage].physicalPage == -2) {
        unsigned physicalPage = memoryManager->AllocMemory(this, virtualPage);
        pageTable[virtualPage].physicalPage = physicalPage;
        unsigned address = (virtualAddress / PAGE_SIZE) * PAGE_SIZE;
        for (unsigned i = 0; i < PAGE_SIZE; i++) {
            // copy to the alloc(ed) memory the PAGE
            swapFile->ReadAt(&mainMemory[physicalPage*PAGE_SIZE + i], 1, address++);
        }
    }
    pageTable[virtualAddress / PAGE_SIZE].dirty = false;
    pageTable[virtualAddress / PAGE_SIZE].valid = true;
}

//----------------------------------------------------------------------
// Chequea si la entry esta o no en la tlb
//----------------------------------------------------------------------
int
isInTLB(TranslationEntry entry)
{
    int inTLB = -1;
    for (unsigned i = 0; i < TLB_SIZE; i++) {
        if (machine->GetMMU()->tlb[i].physicalPage == entry.physicalPage) {
            inTLB = i;
        }
    }
    return inTLB;
}

//----------------------------------------------------------------------
// Borra una pagina de la RAM y la escribe en la swap
//----------------------------------------------------------------------
void
AddressSpace::UnloadPage(unsigned virtualPage)
{
    int inTLB = isInTLB(pageTable[virtualPage]);
    // Si la pagina esta en la tlb la limpio primero
    if (inTLB != -1 && machine->GetMMU()->tlb[inTLB].valid)
        machine->GetMMU()->tlb[inTLB].valid = false;  // clean the tlb
    WriteToSwap(virtualPage); // Siempre va a la swap
    pageTable[virtualPage].valid = false;
}

//----------------------------------------------------------------------
// Escribe una pagina en swap
//----------------------------------------------------------------------
void
AddressSpace::WriteToSwap(unsigned virtualPage)
{
    if (!pageTable[virtualPage].valid) return;
    char *mainMemory = machine->GetMMU()->mainMemory;
    unsigned physicalAddress = pageTable[virtualPage].physicalPage * PAGE_SIZE;
    unsigned virtualAddress = virtualPage * PAGE_SIZE;
    for (unsigned i = 0; i < PAGE_SIZE; i++) {
        // copy the mainMemory to the swap
        swapFile->WriteAt(&mainMemory[physicalAddress + i], 1, virtualAddress++);
    }
    // Seteo que esta pagina esta en swap
    pageTable[virtualPage].physicalPage = -2;
}
