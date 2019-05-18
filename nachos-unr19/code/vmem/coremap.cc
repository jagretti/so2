#include "coremap.hh"

Coremap::Coremap(unsigned numPhyPages)
{
    unsigned size = numPhyPages;
    VirtualEntry *virtualMem = new VirtualEntry[numPhyPages];
}

void
Coremap::AllocMemory()
{
    static int position = 0;
    int dir = userProgramFrameTable->Find();
    if (dir != -1) {
        return dir;
    }
    VirutalEntry toDelete = virtualMem[postion];
    AddresSpace *space = toDelete->space;
    space->writeToSwap(toDelete->virtualPage);
}
