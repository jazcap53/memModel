#ifndef AAJ_PAGE_TABLE_H
#define AAJ_PAGE_TABLE_H

#include <array>
#include <algorithm>
#include <cstdint>
#include "fileShifter.h"
#include "myMemory.h"
#include "aJTypes.h"
#include "aJUtils.h"

namespace MemModel {

    /********************
      There is one PgTabEntry for each slot in the std::array PageTable::PgTab. 
      Each PgTabEntry corresponds to a page in memory, and holds:
      -- the number of the disk block whose data is currently held in that page,
      -- the index of the slot in memory occuped by that page, and 
      -- the time the page was last accessed 
    *********************/

    // Note: we want move construct and move assign for PgTabEntry, so that 
    //       make_heap() in PageTable class will be as efficient as possible.
    struct PgTabEntry {
        PgTabEntry() : PgTabEntry(0UL, 0U, 0UL) {}
        PgTabEntry(bNum_t bN, uint32_t mSIx, uint64_t aT) :
            blockNum(bN), memSlotIx(mSIx), accTime(aT) {}
        PgTabEntry(const PgTabEntry &) = default;
        PgTabEntry(PgTabEntry &&) = default;
        PgTabEntry &operator=(const PgTabEntry &) = default;
        PgTabEntry &operator=(PgTabEntry &&) = default;
        bNum_t blockNum;
        uint32_t memSlotIx;
        uint64_t accTime;
    };

    bool pTEComp(const PgTabEntry &l, const PgTabEntry &r);  // comparator

    /********************
      The page table is maintained as a min heap, ordered by the pTEComp 
      function above. The min heap is held in private std::array<> member pgTab.
      Private member heapSize tracks the number of array entries that are 
      currently part of the heap. 
    ********************/

    class PageTable {
        friend class MemMan;

        PageTable &operator=(const PageTable &) = delete;
        void updateATime(uint32_t loc);
        void resetATime(uint32_t loc);
        PgTabEntry &getPgTabEntry(uint32_t loc) { return pgTab[loc]; }
        const PgTabEntry &getPgTabEntry(uint32_t loc) const 
        { return pgTab[loc]; }
        uint32_t getPgTabSlotFrmMemSlot(uint32_t memSlot) const;
        bool isLeaf(uint32_t memSlot) const { return memSlot >= heapSize / 2; }
        void heapify();
        uint32_t getHeapSize() const { return heapSize; }
        void doPopHeap(PgTabEntry &popped);
        void doPushHeap(PgTabEntry newEntry);
        bool checkHeap() const;
        void setPgTabFull() { pgTabFull = true; }
        bool getPgTabFull() const { return pgTabFull; }
        void print() const;

        uint32_t heapSize = 0U;
        bool pgTabFull = false;
        std::array<PgTabEntry, u32Const::NUM_MEM_SLOTS> pgTab;
        mutable Tabber tabs;
    };

}

#endif
