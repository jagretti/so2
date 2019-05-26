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
Coremap::AllocMemory(AddressSpace *addrSpace, int virtualPage)
{
    static int position = 0;
    int pageNum = memoryMap->Find();
    if (pageNum == -1) {
        // printf("YATTTTAAAAAAAAAAAAAAAAAAAAA\n");
        position = (position + 1) % size;
        pageNum = position;
        AddressSpace *entryAddrSpace = virtualMem[pageNum].addressSpace;
        int entryVirtualPage = virtualMem[pageNum].virtualPage;
        entryAddrSpace->UnloadPage(entryVirtualPage);
    }
    virtualMem[pageNum].virtualPage = virtualPage;
    virtualMem[pageNum].addressSpace = addrSpace;
    return pageNum;
}

void
Coremap::FreeMemory(unsigned int virtualPage)
{
    memoryMap->Clear(virtualPage);
    // TODO free virtualMem
}
