#include <chrono>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "pageTable.h"
#include "simDisk.h"

using namespace MemModel;
using namespace std::chrono;

using std::find_if;
using std::make_heap;  using std::pop_heap;  using std::push_heap;
using std::is_heap;
using std::clog;  using std::endl;  using std::cerr;
using std::setw;

namespace MemModel {

    // Function for ordering the pgTab heap
    // Used by: std::push_heap(), std::pop_heap(), std::is_heap()
    bool pTEComp(const PgTabEntry &l, const PgTabEntry &r)
    {
        return r.accTime < l.accTime;
    }

}

//////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS of PageTable
//////////////////////////////////////////

// Called by: MemMan::rdInSlot(), MemMan::wrtInSlot()
void PageTable::updateATime(uint32_t loc)
{
    pgTab[loc].accTime = getCurTime();
}

// Called by: MemMan::evictThisPage()
void PageTable::resetATime(uint32_t loc)
{
    pgTab[loc].accTime = 0UL;
}

// Given a MemSlot, return the corresponding PgTabSlot
// Called by: MemMan::rdInSlot(), MemMan::wrtInSlot()
uint32_t PageTable::getPgTabSlotFrmMemSlot(uint32_t memSlot) const
{
    auto f = [memSlot](const PgTabEntry &pTE) 
        { return pTE.memSlotIx == memSlot; };

    //    return find_if(pgTab.begin(), pgTab.end(), f) - pgTab.begin();

    auto loc = find_if(pgTab.begin(), pgTab.begin() + heapSize, f);

    return loc - pgTab.begin();
}

void PageTable::heapify()
{
    make_heap(pgTab.begin(), pgTab.begin() + heapSize, pTEComp); 
}    

// Called by: MemMan::evictLRUPage()
void PageTable::doPopHeap(PgTabEntry &popped)
{ 
    assert(heapSize);

    pop_heap(pgTab.begin(), pgTab.begin() + heapSize, pTEComp);
    --heapSize;
    popped = pgTab[heapSize];
}

// Called by: MemMan::rdPgFrmDsk()
void PageTable::doPushHeap(PgTabEntry newEntry)
{
    assert(heapSize < NUM_MEM_SLOTS);

    pgTab[heapSize] = newEntry;
    push_heap(pgTab.begin(), pgTab.begin() + heapSize, pTEComp);
    ++heapSize;
}

// Called by: MemMan::evictLRUPage(), MemMan::rdPgFrmDsk()
bool PageTable::checkHeap() const
{
    return is_heap(pgTab.begin(), pgTab.begin() + heapSize, pTEComp);
}

// Called by: MemMan::evictLRUPage()
void PageTable::print() const
{
    clog << '\n' << tabs(1,true) << "Contents of page table:\n";
    uint32_t i = 0U;
    for (auto p : pgTab) { 
        clog << tabs(2, true) << "pgTblIx: " << setw(2) << i << "   block: " 
             << p.blockNum 
             << "   memSlotIx: " << p.memSlotIx << "   accTime: " 
             << setw(6) << p.accTime << '\n';
        ++i;
    }
}
