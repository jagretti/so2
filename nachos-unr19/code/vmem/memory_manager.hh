#ifndef NACHOS_VMEM_MEMORY_MANAGER__HH
#define NACHOS_VMEM_MEMORY_MANAGER__HH

#include "lib/bitmap.hh"
#include "machine/mmu.hh"
#include "vmem/coremap.hh"
#include "threads/synch.hh"


class MemoryManager {

public:
    MemoryManager(Coremap *coremap, Bitmap *userProgramFrameTable);
    ~MemoryManager();
    int AllocMemory(AddressSpace *addressSpace, unsigned virtualPage);
    void FreeMemory(unsigned virtualPage);
    void CleanMemory(AddressSpace *addressSpace, unsigned index);
    unsigned GetPageNumQueue(unsigned queue_page);
    unsigned GetPageNumLRU();
    void printCoremap();

private:
    Coremap *coremap;
    Bitmap *bitmap;
};
#endif // NACHOS_VMEM_MEMORY_MANAGER__HH
