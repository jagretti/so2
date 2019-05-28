#include "vmem/memory_manager.hh"

//-------------------------------------------------
// Clase que contiene al coremap y bitmap
// para funcionalidades de Virtual Memory
//-------------------------------------------------
MemoryManager::MemoryManager(Coremap *cm, Bitmap *bm)
{
    coremap = cm;
    bitmap = bm; // userProgramframetable
    lock = new Lock("MML");
}

MemoryManager::~MemoryManager() 
{
    delete lock;
}

//-------------------------------------------------
// AllocMemory: Alloca un bloque de memoria para
// una nueva pagina, si no hay bloques libres, libera
// uno y lo asigna, enviando la pagina eliminada a
// swap
//-------------------------------------------------
int
MemoryManager::AllocMemory(AddressSpace *addrSpace, unsigned virtualPage)
{
    lock->Acquire();
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
    lock->Release();
    return pageNum;
}

//-------------------------------------------------
// FreeMemory: Libera la pagina del address space
// y del coremap
//-------------------------------------------------
void
MemoryManager::FreeMemory(unsigned virtualPage)
{
    AddressSpace *entryAddrSpace = coremap[virtualPage].addressSpace;
    int entryVirtualPage = coremap[virtualPage].virtualPage;
    entryAddrSpace->UnloadPage(entryVirtualPage);
}
