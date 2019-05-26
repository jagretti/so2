#ifndef NACHOS_VMEM_MEMORY_MANAGER__HH
#define NACHOS_VMEM_MEMORY_MANAGER__HH

#include "vmem/coremap.hh"
#include "machine/mmu.hh"

class MemoryManager {

public:
    MemoryManager(Coremap *coremap);
    ~MemoryManager();
    int AllocMemory(AddressSpace *addressSpace, unsigned virtualPage);
    void FreeMemory(unsigned virtualPage);

private:
    Coremap *coremap;
};
#endif // NACHOS_VMEM_MEMORY_MANAGER__HH
