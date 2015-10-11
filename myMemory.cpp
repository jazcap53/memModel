#include "myMemory.h"

using namespace MemModel;

Memory::Memory()
{
    avlMemSlts.set();
}

// Returns index of first free memory slot, if any.
// Otherwise, returns NUM_MEM_SLOTS
// Called by: MemMan::processRequest()
uint32_t Memory::getFirstAvlMemSlt()
{
    uint32_t crsr = 0U;

    while (crsr != NUM_MEM_SLOTS) {
        if (avlMemSlts[crsr])
            break;
        ++crsr;
    }

    if (crsr != NUM_MEM_SLOTS)
        avlMemSlts.reset(crsr);

    return crsr;
}

bool Memory::makeAvlMemSlt(uint32_t memSlt)
{
    bool aOk = false;

    if (!avlMemSlts.test(memSlt)) {
        avlMemSlts.set(memSlt);
        aOk = true;
    }

    return aOk;
}

bool Memory::takeAvlMemSlt(uint32_t memSlt)
{
    bool aOk = false;

    if (avlMemSlts.test(memSlt)) {
        avlMemSlts.reset(memSlt);
        aOk = true;
    }

    return aOk;
}

// Called by: MemMan::rdPgFrmDsk(), MemMan::wrtPgToDsk(),
//            MemMan::updatePgInMem()
Page *Memory::getPage(uint32_t ix)
{ 
    return &theMem.at(ix); 
}
