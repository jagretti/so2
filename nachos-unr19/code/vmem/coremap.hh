#ifndef NACHOS_VMEM_COREMAP__HH
#define NACHOS_VMEM_COREMAP__HH

#include "userprog/address_space.hh"

class Coremap {
public:
    AddressSpace *addressSpace;
    unsigned virtualPage;
};

#endif // NACHOS_VMEM_COREMAP__HH
