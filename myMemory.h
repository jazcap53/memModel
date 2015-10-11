#ifndef AAJ_PAGE_AND_MEMORY_CLASSES_H
#define AAJ_PAGE_AND_MEMORY_CLASSES_H

#include <array>
#include <string>
#include <cassert>
#include <bitset>
#include "aJTypes.h"

namespace MemModel {

    /*****************************
      Represents a page in memory
    *****************************/
    struct Page {
        Page(): dat{0U} { }

        unsigned char dat[u32Const::BYTES_PER_PAGE]; 
    };

    /*****************************
     Represents memory as an array of Page's
    *****************************/
    class Memory {
        friend class MemMan;
        
    public:
        Memory();
        Memory &operator=(const Memory &) = delete;

    private:
        uint32_t getFirstAvlMemSlt();
        bool makeAvlMemSlt(uint32_t memSlt);
        bool takeAvlMemSlt(uint32_t memSlt);
        Page *getPage(uint32_t ix);

        uint32_t firstAvlMemSlt = 0;
        std::array<Page, u32Const::NUM_MEM_SLOTS> theMem{}; 
        std::bitset<u32Const::NUM_MEM_SLOTS> avlMemSlts;
    };

}

#endif
