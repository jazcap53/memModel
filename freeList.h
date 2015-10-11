#ifndef AAJ_THE_FREE_LIST_H
#define AAJ_THE_FREE_LIST_H

#include <fstream>
#include <string>
#include "aJTypes.h"
#include "aJUtils.h"
#include "arrBit.h"

namespace FileSys {
    
    /*****************************
      This class maintains the free list as a pair of ArrBit members and a 
      position indicator; there is a bit for each block in the file system.
      A set bit indicates an available block.
         Algorithm:
            When the file manager requests a block, the bit in bitsFrm at 
               position fromPosn is cleared, fromPosition is incremented, and
               the index of the cleared bit is returned. 
            When the file manager indicates that a block is no longer in use,
               the corresponding bit in bitsTo is set.
            When all of bitsFrm has been used, bitsFrm |= bitsTo and bitsTo is 
               cleared.
        Rationale: 
           All other things being equal, the blocks that were first put into use
              will tend to be the first given back to the free list. 
           After all blocks have been used once, the strategy tries to access
              the blocks in the order of their likelihood of having been
              replaced into the list. 
           This may reduce fragmentation, at least in the short term.
    *****************************/
    class FreeList {
        friend class FileMan;

    private:
        explicit FreeList(std::string ffn);
        ~FreeList();

        void loadLst();
        void storeLst();
        bNum_t getBlk();              // returns SENTINEL_BNUM if no free blocks
        void putBlk(bNum_t tgt);
        void refresh();

        std::fstream frs;

        ArrBit<u32Const, u32Const, bNum_t, NUM_FREE_LIST_BLOCKS, 
               BITS_PER_PAGE> bitsFrm;
        ArrBit<u32Const, u32Const, bNum_t, NUM_FREE_LIST_BLOCKS, 
               BITS_PER_PAGE> bitsTo;
        
        bNum_t fromPosn = 0U;
        Tabber tabs;
    };

}

#endif
