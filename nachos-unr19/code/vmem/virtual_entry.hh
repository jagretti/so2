#ifndef NACHOS_VMEM_VIRTUAL_ENTRY__HH
#define NACHOS_VMEM_VIRTUAL_ENTRY__HH

#include "userprog/address_space.hh"

class VirtualEntry {
public:
    VirtualEntry();
    ~VirtualEntry();
    AddressSpace *addressSpace;
    int virtualPage;

};

#endif // NACHOS_VMEM_VIRTUAL_ENTRY__HH
