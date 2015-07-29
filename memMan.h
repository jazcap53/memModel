#ifndef AAJ_MEMORY_MANAGEMENT_CLASS_H
#define AAJ_MEMORY_MANAGEMENT_CLASS_H

#include <bitset>
#include <map>
#include <iostream>
#include <limits>
#include <cstdint>
#include <fstream>
#include <memory>
#include <utility>
#include "pageTable.h"
#include "simDisk.h"
#include "change.h"
#include "fileMan.h"
#include "aJTypes.h"

    /***************************
      The MemMan class is the controlling class for the memory elements of the
      program. MemMan obeys instructions from the client sent, via the file 
      manager, to processRequest().

      A call to MemMan::processRequest() behaves as follows.
          -- If the requested page (cg.blkNum) is not in memory:
          --     If there is no available slot in the page table:
          --         Evict a page from page table 
          --         Return the slot from which a page was evicted
          --     Copy the requested page into memory from the simulated disk
          --     Copy in any changes to that page from the change log 
          -- If any lines have been altered in the argument cg:
          --     Write the requested page
          -- Else:
          --     Read the requested page

      The page table is maintained as a min heap, ordered on the last access 
      time of the page. 

      The page replacement algorithm is LRU. 

      The actions of the simulator may be monitored by reading from file 
      output.txt.
    ***************************/

namespace MemModel {
    using namespace Disk;
    using namespace FileSys;

    class MemMan {
        friend class FileSys::FileMan;
        
        // writing change log to journal
		const uint32_t WRITEALL_DELAY_USEC = 25000;
        // writing journal to disk
		const uint32_t JRNL_PURGE_DELAY_USEC = 100000;  
        const uint32_t CHANGE_LOG_FULL = u32Const::BYTES_PER_PAGE * 2U;

        enum { CG_OHEAD = 16U, JRNL_ENTRY_OHEAD = 24U };

    public:
        MemMan(Memory *pm, SimDisk *pd, Journal *pj, ChangeLog *pcl,
               Status *pstt, bool v = false);
        MemMan(const MemMan &) = delete;
        MemMan &operator=(const MemMan &) = delete;
        MemMan(MemMan &&) = delete;
        MemMan &operator=(MemMan &&) = delete;
        ~MemMan();

    private:
        void processRequest(const Change &cg, FileMan *p_fM);
        bool getReqType(const Change &cg, bool inMem) const;
        void mkPgReady(bNum_t bNum, bool inMem);
        void timedActs(bool aWrite, bNum_t bNum, FileMan *p_fM);
        uint64_t getSzJrnlWrt() const;
        uint32_t setupPg(bNum_t bNum);
        void updatePgInMem(bNum_t bNum, uint32_t memSlot);
        uint32_t evictLRUPage();
        void evictThisPage(bNum_t bNum);
        void rdPgFrmDsk(bNum_t bNum, uint32_t memSlot);
        void rdInSlot(bNum_t bNum);
        void wrtInSlot(const Change &cg);
        bool blkInPgTab(bNum_t bNum) const;
        void debugDisplay();
  
        PageTable pT;
        Memory *p_m;
        SimDisk *p_d;
        Journal *p_j;
        ChangeLog *p_cL;
        Status *p_stt;
        bool verbose;

        std::bitset<bNum_tConst::NUM_DISK_BLOCKS> blksInMem = 0UL;
        // map: (block number) => (slot in memory)
        std::map<bNum_t, uint32_t> blkLocsInMem;

        mutable Tabber tabs;
    };

}

#endif
