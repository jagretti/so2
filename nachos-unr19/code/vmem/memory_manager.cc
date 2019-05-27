#include "vmem/memory_manager.hh"

MemoryManager::MemoryManager(Coremap *cm, Bitmap *bm)
{
    coremap = cm;
    bitmap = bm; // userProgramframetable
}

MemoryManager::~MemoryManager() {}

int
MemoryManager::AllocMemory(AddressSpace *addrSpace, unsigned virtualPage)
{
    static unsigned pageNum = 0;
    unsigned dir = bitmap->Find();
    if (dir == -1) {
        pageNum = (pageNum + 1) % NUM_PHYS_PAGES;
    } else {
        pageNum = dir;
    }
    if (coremap[pageNum].isAllocated) FreeMemory(pageNum);
    coremap[pageNum].virtualPage = virtualPage;
    coremap[pageNum].addressSpace = addrSpace;
    coremap[pageNum].isAllocated = true;
    return pageNum;
}

void
MemoryManager::FreeMemory(unsigned virtualPage)
{
    AddressSpace *entryAddrSpace = coremap[virtualPage].addressSpace;
    int entryVirtualPage = coremap[virtualPage].virtualPage;
    entryAddrSpace->UnloadPage(entryVirtualPage);
}
