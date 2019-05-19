#include "coremap.hh"

Coremap::Coremap(unsigned numPhyPages)
{
    unsigned size = numPhyPages;
    VirtualEntry *virtualMem = new VirtualEntry[numPhyPages];
    Bitmap *memoryMap = new Bitmap(numPhyPages);
}

Coremap::~Coremap
{
    delete []virtualMem;
    delete memoryMap;
}

int
Coremap::AllocMemory()
{
    static int position = 0;
    int pageNum = memoryMap->Find();
    if (pageNum != -1) {
        return page;
    }
    pageNum = position % size;
    position++;
    VirutalEntry toDelete = virtualMem[pageNum];
    AddresSpace *space = toDelete->space;
    space->writeToSwap(toDelete->virtualPage);
    return pageNum;
}

void
Coremap::FreeMemory(unsigned int virtualPage)
{
    memoryMap->Clear(virtualPage);
}
