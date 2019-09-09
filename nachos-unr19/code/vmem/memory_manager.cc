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
    // lock->Acquire();
    static unsigned queue_page = 0;
    queue_page = (queue_page + 1) % NUM_PHYS_PAGES;
    unsigned pageNum = GetPageNumQueue(queue_page);
    if (coremap[pageNum].isAllocated) FreeMemory(pageNum);
    coremap[pageNum].virtualPage = virtualPage;
    coremap[pageNum].addressSpace = addrSpace;
    coremap[pageNum].isAllocated = true;
    // lock->Release();
    DEBUG('l', "MemoryManager::AllocMemory %d \n", virtualPage);
    return pageNum;
}

//-------------------------------------------------
//
//-------------------------------------------------
unsigned
MemoryManager::GetPageNumQueue(unsigned queue_page)
{
    unsigned dir = bitmap->Find();
    unsigned page;
    if (dir == -1) {
        page = queue_page;
    } else {
        page = dir;
    }
    return page;
}

//-------------------------------------------------
//
//-------------------------------------------------
unsigned
MemoryManager::GetPageNumLRU()
{
    unsigned victim = queue_page;
    while (coremap[victim].addressSpace->GetEntry(coremap[victim]).use == 1) {
        coremap[victim].addressSpace->GetEntry(coremap[victim]).use = 0;
        victim = (victim + 1) % NUM_PHYS_PAGES;
    }
    queue_page = victim; // que hacemos con el queue_page? cual usamos?
    queue_page = (queue_page + 1) % NUM_PHYS_PAGES; // +1 para la proxima vez
    return victim;
}

//-------------------------------------------------
// FreeMemory: Libera la pagina del address space
// y del coremap
//-------------------------------------------------
void
MemoryManager::FreeMemory(unsigned virtualPage)
{
    DEBUG('l', "MemoryManager::FreeMemory %d \n", virtualPage);
    AddressSpace *entryAddrSpace = coremap[virtualPage].addressSpace;
    int entryVirtualPage = coremap[virtualPage].virtualPage;
    entryAddrSpace->UnloadPage(entryVirtualPage);
    coremap[virtualPage].isAllocated = false;
}

void
MemoryManager::CleanMemory(AddressSpace *addr, unsigned page)
{
    ASSERT(coremap[page].addressSpace == addr);
    DEBUG('l', "MemoryManager::CleanMemory %d \n", page);
//    bitmap->Clear(page);
    coremap[page].isAllocated = false;
}
