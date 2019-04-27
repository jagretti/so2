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

    exec = executable;

    noffHeader noffH;
    exec->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFF_MAGIC &&
          WordToHost(noffH.noffMagic) == NOFF_MAGIC)
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFF_MAGIC);

    // How big is address space?

    unsigned size = noffH.code.size + noffH.initData.size
                    + noffH.uninitData.size + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    ASSERT(numPages <= NUM_PHYS_PAGES);
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    //
    isValid = numPages <= userProgramFrameTable->CountClear();

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];
    //
    char *mainMemory = machine->GetMMU()->mainMemory;
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = -1; //userProgramFrameTable->Find();
        pageTable[i].valid        = false;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
        // Zero out the physical address space
        //memset(&(mainMemory[pageTable[i].physicalPage * PAGE_SIZE]), 0, PAGE_SIZE);
    }
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    for (unsigned i = 0; i < numPages; i++) {
        userProgramFrameTable->Clear(pageTable[i].physicalPage);
    }
    delete [] pageTable;
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
{}

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

void
AddressSpace::SaveEntry(TranslationEntry toSave)
{
    int vpn = toSave.virtualPage;
    this->pageTable[vpn] = toSave;
}


void
AddressSpace::LoadPage(unsigned virtualAddr)
{
    noffHeader noffH;
    exec->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFF_MAGIC &&
        WordToHost(noffH.noffMagic) == NOFF_MAGIC)
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFF_MAGIC);
    
    int physicalPage = userProgramFrameTable->Find();

    char b;
    int read;
    for(int byte = 0; byte < PAGE_SIZE; byte++) {
        int vaddr = vaddr + byte;
        int paddr = physicalPage * PAGE_SIZE + byte;
        // donde esta la cosa
        if (vaddr >= noffH.code.virtualAddr && vaddr < noffH.code.virtualAddr + noffH.code.size) {
            // code
            //exec->ReadAt(&(machine->GetMMU()->mainMemory[paddr]), 1, noffH.code.inFileAddr + vaddr - noffH.code.virtualAddr);
            read = exec->ReadAt(&(b), 1, noffH.code.inFileAddr + vaddr - noffH.code.virtualAddr);
            DEBUG('l', "ReadAt: read = %d - Byte leido: b = %s\n", read, b);  
            machine->GetMMU()->mainMemory[paddr] = b;
        } else if (vaddr >= noffH.initData.virtualAddr && vaddr < noffH.initData.virtualAddr + noffH.initData.size) {
            // initData
            //read = exec->ReadAt(&(machine->GetMMU()->mainMemory[paddr]), 1, noffH.initData.inFileAddr + vaddr - noffH.initData.virtualAddr);
            read = exec->ReadAt(&(b), 1, noffH.initData.inFileAddr + vaddr - noffH.initData.virtualAddr);
            DEBUG('l', "ReadAt: read = %d - Byte leido: b = %s\n", read, b);  
            machine->GetMMU()->mainMemory[paddr] = b;
        } else {
            machine->GetMMU()->mainMemory[paddr] = 0;
        }
    }
    int virtualPageNum = virtualAddr / PAGE_SIZE;
    // validamos la pagina en la pageTable
    pageTable[virtualPageNum].physicalPage = physicalPage;
    // le ponemos que la pagina es valida
    pageTable[virtualPageNum].valid = true;
}
