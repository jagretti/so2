#ifndef NACHOS_VMEM_COREMAP__HH
#define NACHOS_VMEM_COREMAP__HH

#include "userprog/address_space.hh"

class Coremap {
public:
    AddressSpace *addressSpace;
    unsigned virtualPage;
    bool isAllocated;
    bool inUse;
};

#endif // NACHOS_VMEM_COREMAP__HH
