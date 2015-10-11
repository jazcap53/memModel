#ifndef AAJ_MEMMODEL_WIPE_LIST_H
#define AAJ_MEMMODEL_WIPE_LIST_H

#include "aJTypes.h"
#include "arrBit.h"

namespace FileSys  {
    class FileMan;
}

namespace MemModel {
    class MemMan;
}

namespace Disk {

    /********************
      Implements cleaning used disk blocks in a lazy fashion.
      Data member 'dirty' tracks the numbers of blocks that have been written
      to, but are no longer being used. These blocks are wiped clean before they
      are allocated to a different file, or when the number of dirty blocks
      exceeds a threshhold value.
    ********************/
    class WipeList {
        friend class Journal;

        const uint32_t DIRTY_BEFORE_WIPE = 16U;

    public:
                
        WipeList();
        void setDirty(bNum_t bNum);
        bool isDirty(bNum_t bNum);
        void clearArray();
        bool isRipe();

    private:
        
        ArrBit<u32Const, u32Const, bNum_t, NUM_WIPE_PAGES, 
               BITS_PER_PAGE> dirty;
    };
}

#endif
