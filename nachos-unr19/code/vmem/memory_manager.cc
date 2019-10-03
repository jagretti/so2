#include "vmem/memory_manager.hh"

//-------------------------------------------------
// Clase que contiene al coremap y bitmap
// para funcionalidades de Virtual Memory
//-------------------------------------------------
MemoryManager::MemoryManager(Coremap *cm, Bitmap *bm)
{
    coremap = cm;
    bitmap = bm; // userProgramframetable
}

MemoryManager::~MemoryManager()
{

}

void
MemoryManager::printCoremap()
{
    printf("--------------\n");
    for(unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
        printf("COREMAP[%d] -- ", i);
        printf("AddressSpace: %p VirtualPage: %d isAllocated: %d inUse %d\n",
            coremap[i].addressSpace,
            coremap[i].virtualPage,
            coremap[i].isAllocated,
            coremap[i].inUse);
    }
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
    static unsigned queue_page = 0;
    queue_page = (queue_page + 1) % NUM_PHYS_PAGES;
    //unsigned pageNum = GetPageNumQueue(queue_page);
    unsigned pageNum = GetPageNumLRU();
    if (coremap[pageNum].isAllocated) FreeMemory(pageNum);
    coremap[pageNum].virtualPage = virtualPage;
    coremap[pageNum].addressSpace = addrSpace;
    coremap[pageNum].isAllocated = true;
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
    static unsigned queue_page = 0;
    unsigned victim = queue_page;
    // 1- search for an empty coremap frame
    for (unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
	if (!coremap[i].isAllocated) {
            DEBUG('p', "MemoryManager::GetPageNumLRU %d \n", i);
            return i;
	 }
    }
    // 2- Search for an unused TranslationEntry
    unsigned i = 0;
    while (coremap[victim].addressSpace->GetEntry(coremap[victim].virtualPage)->use && i < NUM_PHYS_PAGES) {
        coremap[victim].addressSpace->GetEntry(coremap[victim].virtualPage)->use = 0;
        victim = (victim + 1) % NUM_PHYS_PAGES;
	    i++;
    }
    if (i == NUM_PHYS_PAGES) { // iterated over all the coremap and didn't found an unsed TranslationEntry
        // 3- No empty frame or unsued so one must be chosen to be freed
        i = 0;
        while (coremap[victim].addressSpace->GetEntry(coremap[victim].virtualPage)->dirty && i < NUM_PHYS_PAGES) {
            // don't change the dirty value
            victim = (victim + 1) % NUM_PHYS_PAGES;
            i++; // at most NUM_PHYS_PAGES
        }
    }

    queue_page = victim; // update the queue with the current victim/position
    queue_page = (queue_page + 1) % NUM_PHYS_PAGES; // update the queue's head
    DEBUG('p', "MemoryManager::GetPageNumLRU %d \n", victim);
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
