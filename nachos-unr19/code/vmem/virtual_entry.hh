#ifndef NACHOS_VMEM_VIRTUAL_ENTRY__HH
#define NACHOS_VMEM_VIRTUAL_ENTRY__HH

class VirtualEntry {
public:
    VirtualEntry(unsigned numPhyPages);
    ~VirtualEntry();
    AddressSpace *addressSpace;
    int virtualPage;

}

#endif // NACHOS_VMEM_VIRTUAL_ENTRY__HH
