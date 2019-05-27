#ifndef NACHOS_VMEM_MEMORY_MANAGER__HH
#define NACHOS_VMEM_MEMORY_MANAGER__HH

#include "lib/bitmap.hh"
#include "machine/mmu.hh"
#include "vmem/coremap.hh"


class MemoryManager {

public:
    MemoryManager(Coremap *coremap, Bitmap *userProgramFrameTable);
    ~MemoryManager();
    int AllocMemory(AddressSpace *addressSpace, unsigned virtualPage);
    void FreeMemory(unsigned virtualPage);

private:
    Coremap *coremap;
    Bitmap *bitmap;
};
#endif // NACHOS_VMEM_MEMORY_MANAGER__HH
