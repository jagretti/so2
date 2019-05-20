#include "coremap.hh"

Coremap::Coremap(unsigned numPhyPages)
{
    size = numPhyPages;
    virtualMem = new VirtualEntry[size];
    memoryMap = new Bitmap(size);
}

Coremap::~Coremap()
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
        return pageNum;
    }
    printf("YATTTTAAAAAAAAAAAAAAAAAAAAA\n");
    position = (position + 1) % size;
    pageNum = position;
    VirtualEntry toDelete = virtualMem[pageNum];
    AddressSpace *addressSpace = toDelete.addressSpace;
    int virtualPage = toDelete.virtualPage;
    addressSpace->WriteToSwap(virtualPage);
    return pageNum;
}

void
Coremap::FreeMemory(unsigned int virtualPage)
{
    memoryMap->Clear(virtualPage);
}
