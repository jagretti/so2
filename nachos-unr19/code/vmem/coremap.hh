#ifndef NACHOS_VMEM_COREMAP__HH
#define NACHOS_VMEM_COREMAP__HH

class Coremap {
 public:
    Coremap(unsigned numPhyPages);
    ~Coremap();
    int AllocMemory();
    void FreeMemory(unsigned virtualPage);
}

#endif // NACHOS_VMEM_COREMAP__HH
