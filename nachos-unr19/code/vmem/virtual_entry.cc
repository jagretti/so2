#include "vmem/virtual_entry.hh"

VirtualEntry::VirtualEntry() {
    addressSpace = nullptr;
    virtualPage = -1;
}

VirtualEntry::~VirtualEntry() {
    // ???
}
