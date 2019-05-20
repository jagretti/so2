#ifndef NACHOS_VMEM_COREMAP__HH
#define NACHOS_VMEM_COREMAP__HH

#include "lib/bitmap.hh"
#include "virtual_entry.hh"

class Coremap {
 public:
    Coremap(unsigned numPhyPages);
    ~Coremap();
    int AllocMemory();
    void FreeMemory(unsigned virtualPage);
private:
    unsigned size;
    VirtualEntry *virtualMem;
    Bitmap *memoryMap;
};

#endif // NACHOS_VMEM_COREMAP__HH
