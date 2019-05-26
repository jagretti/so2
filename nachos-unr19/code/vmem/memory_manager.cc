#include "vmem/memory_manager.hh"

MemoryManager::MemoryManager(Coremap *cm)
{
    coremap = cm;
}

MemoryManager::~MemoryManager() {}

int
MemoryManager::AllocMemory(AddressSpace *addrSpace, unsigned virtualPage)
{
    static unsigned pageNum = 0;
    pageNum = (pageNum + 1) % NUM_PHYS_PAGES;
    FreeMemory(pageNum);
    coremap[pageNum].virtualPage = virtualPage;
    coremap[pageNum].addressSpace = addrSpace;
    return pageNum;
}

void
MemoryManager::FreeMemory(unsigned virtualPage)
{
    AddressSpace *entryAddrSpace = coremap[virtualPage].addressSpace;
    int entryVirtualPage = coremap[virtualPage].virtualPage;
    entryAddrSpace->UnloadPage(entryVirtualPage);
}
