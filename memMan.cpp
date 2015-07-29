#include <iostream>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <cassert>
#include <stdexcept>
#include <limits>
#include <memory>
#include <utility>
#include <bitset>
#include <array>
#include <list>
#include <map>
#include "memMan.h"
#include "myMemory.h"
#include "simDisk.h"
#include "change.h"
#include "status.h"
#include "crc.h"
#include "aJTypes.h"

using std::clog;  using std::endl;
using std::right;  using std::flush;  using std::ios_base;
using std::setw;
using std::copy;
using std::fstream;
using std::ostream_iterator;
using std::length_error;
using std::numeric_limits;
using std::allocator;
using std::pair;
using std::bitset;
using std::array;
using std::list;
using std::map;

using namespace MemModel;
using namespace CRC;

////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////

// v defaults to false
// Called by: main()
MemMan::MemMan(Memory *pm, SimDisk *pd, Journal *pj, ChangeLog *pcl, 
               Status *pstt, bool v):
    p_m(pm), p_d(pd), p_j(pj), p_cL(pcl), p_stt(pstt), verbose(v)
{
    // change write timer starts now; handles case of slow program startup
    p_cL->lastCgWrtTime = getCurTime(); 
    p_stt->wrt("Running");
}

MemMan::~MemMan()
{ 
    clog << "\n\tProgram exiting..." << '\n';

    p_j->wrtCgLogToJrnl(*p_cL);
    p_j->purgeJrnl(false);          // false: don't keep going; program finished
}

////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////

// Calls functions to read or write a page.
// Called by: FileMan::submitRequest()
void MemMan::processRequest(const Change &cg, FileMan *p_fM)
{
    assert(pT.heapSize == p_m->avlMemSlts.size() - p_m->avlMemSlts.count());

    bNum_t bNum = cg.blkNum;

    assert(bNum < NUM_DISK_BLOCKS);

    bool inMem = blksInMem.test(bNum);

    bool aWrite = getReqType(cg, inMem);

    // ensure page is in memory; update the page from cgLog if needed
    mkPgReady(bNum, inMem);

    // wrtInSlot() writes new change to that page.
    if (aWrite)
        wrtInSlot(cg);
    else
        rdInSlot(bNum);
    
    // may save inode table, free list, change log, may purge journal,
    // depending on whether timers have expired
    timedActs(aWrite, bNum, p_fM);
}

// Determine if request is a read or a write
// Set its timestamp
// Log to activity trace on stdout
// Called by: processRequest()
bool MemMan::getReqType(const Change &cg, bool inMem) const
{
    // if there are any altered lines, we write
    bool aWrt = cg.linesAltered();

    cg.timeStamp = getCurTime();                     // cg.timeStamp is volatile

    clog << tabs(1, true) << "Request for " 
         << (aWrt ? "write to " : "read from ") 
         << "block " << cg.blkNum << (inMem ? " " : " not ") 
         << "in memory at time " << cg.timeStamp << '\n';    

    return aWrt;
}

// Ensure target page is in memory.
// If necessary, update page with any changes still in change log.
// Called by: processRequest()
void MemMan::mkPgReady(bNum_t bNum, bool inMem)
{
    uint32_t memSlot = SENTINEL_32;

    if (!inMem)
        memSlot = setupPg(bNum);

    if (memSlot != SENTINEL_32)
        updatePgInMem(bNum, memSlot);
}

// If purge timer has expired or change log is full:
//     store inode table
//     store free list
//     write change log
//     purge journal
// Else if change write time has expired:
//     write change log
// Called by: processRequest()
void MemMan::timedActs(bool aWrite, bNum_t bNum, FileMan *p_fM)
{
    if (verbose)
        debugDisplay();

    // Don't want to let the change log get too big.
    uint64_t bytesToJrnl = getSzJrnlWrt();
    if (bytesToJrnl >= CG_LOG_FULL)
        clog << '\n' << tabs(1) << "BytesToJrnl " << bytesToJrnl 
             << " >= CG_LOG_FULL (" << CG_LOG_FULL << ")\n";

    // Don't want too much time to elapse between journal purges
    uint64_t curTime = getCurTime();
    uint64_t elapsed = curTime - p_j->getLastJrnlPurgeTime();
    if (elapsed > JRNL_PURGE_DELAY_USEC)
        clog << "\n\tElapsed " << elapsed << " > JRNL_PURGE_DELAY_USEC ("
             << JRNL_PURGE_DELAY_USEC << ")\n";

    if (elapsed > JRNL_PURGE_DELAY_USEC || bytesToJrnl >= CG_LOG_FULL) { 
        p_fM->doStoreInodes();
        p_fM->doStoreFreeList();
        p_cL->lastCgWrtTime = curTime;
        p_j->wrtCgLogToJrnl(*p_cL);                              // to jrnl_file
        p_j->setLastJrnlPurgeTime(curTime);
        p_j->purgeJrnl();                                        // to disk_file
    }
    else {
        uint64_t delay = curTime - p_cL->lastCgWrtTime;
        if (delay > WRITEALL_DELAY_USEC) {
            clog << '\n' << tabs(1) << "Delay " << delay 
                 << " > WRITEALL_DELAY_USEC (" << WRITEALL_DELAY_USEC << ")\n";
            p_cL->lastCgWrtTime = curTime;
            p_j->wrtCgLogToJrnl(*p_cL);
        }
    }
}

// Called by: timedActs()
uint64_t MemMan::getSzJrnlWrt() const
{
    uint64_t ttlSz = 0UL;

    uint64_t numDataLines = p_cL->getCgLineCt();
    uint64_t dataBytes = numDataLines * BYTES_PER_LINE;

    // Need 1 8-byte selector_t for [0..7] data lines, 
    // 2 selectors for [8..15] lines
    // so [0..7]->8, [8..15]->16, etc.
    uint64_t selectBytes = (numDataLines >> 3) + 8;

    ttlSz += dataBytes + selectBytes + CG_OHEAD;
    ttlSz += JRNL_ENTRY_OHEAD;

    return ttlSz;
}

// Bring page bNum into memory, first evicting a page if necessary.
// Called by: mkPgReady()
uint32_t MemMan::setupPg(bNum_t bNum)
{
    uint32_t memSlot = p_m->getFirstAvlMemSlt();

    if (memSlot == NUM_MEM_SLOTS)
        memSlot = evictLRUPage();

    if (memSlot != SENTINEL_32)
        rdPgFrmDsk(bNum, memSlot); 

    return memSlot;
}

// Called by: mkPgReady()
void MemMan::updatePgInMem(bNum_t bNum, uint32_t slot)
{
    if (p_cL->theLog.count(bNum)) {
        list<Change> lc = p_cL->theLog[bNum];
        for (Change cg : lc)
            p_j->wrtCgToPg(cg, *p_m->getPage(slot));
    }
}

// Moves LRU page out of memory. 
// Called by: setupPage()
uint32_t MemMan::evictLRUPage()
{
    uint32_t memSlot = SENTINEL_32;

    if (pT.heapSize == NUM_MEM_SLOTS) {

        assert(pT.checkHeap());
        
        pT.print();                         // <=== DO NOT DELETE THIS LINE ===<

        PgTabEntry temp;
        
        pT.doPopHeap(temp);

        bNum_t oldPageNum = temp.blockNum;
        memSlot = blkLocsInMem[oldPageNum];
    
        // do our bookkeeping: out with the old
        blkLocsInMem.erase(oldPageNum);
        blksInMem.reset(oldPageNum);

        p_m->makeAvlMemSlt(memSlot);

        clog << tabs(1, true) << "Evicted page " << oldPageNum 
             << " from memory slot " << memSlot << " at time " << getCurTime() 
             << '\n';

        assert(pT.checkHeap());
    }

    return memSlot;
}

// Moves a specified page out of memory. 
// Called by: FileMan::deleteFile(), FileMan::remvBlock()
void MemMan::evictThisPage(bNum_t bNum)
{
    // if bNum is actually in heap: else do nothing
    if (blkInPgTab(bNum)) { 
        assert(pT.checkHeap());

        uint32_t memSlot = blkLocsInMem[bNum];
        // returns pT.heapSize if memSlot not found in page table
        uint32_t pgTabSlot = pT.getPgTabSlotFrmMemSlot(memSlot);

        assert(pgTabSlot != pT.heapSize);

        pT.resetATime(pgTabSlot);        // move PgTab[pgTabSlot] to top of heap

        pT.heapify();

        PgTabEntry dummy;

        pT.doPopHeap(dummy);

        assert(dummy.blockNum == bNum);

        // do our bookkeeping: out with the old
        blkLocsInMem.erase(bNum);
        blksInMem.reset(bNum);

        p_m->makeAvlMemSlt(memSlot);

        // pT.print();                      // <=== DO NOT DELETE THIS LINE ===<
    
        clog << tabs(2, true) << "Evicted page " << bNum 
             << " from memory slot " << memSlot << " at time " << getCurTime() 
             << '\n';
      
        assert(pT.checkHeap());
    }
}

// Brings page bNum from disk into memory at slot memSlot.
// Called by setupPg()
void MemMan::rdPgFrmDsk(bNum_t bNum, uint32_t memSlot)
{
    assert(pT.checkHeap());

    if (memSlot == NUM_MEM_SLOTS - 1)
        pT.setPgTabFull();

    clog << tabs(1) << "Moving page " << bNum << " into memory slot " 
         << memSlot << " at time " << getCurTime() << '\n';

    // Copy block from disk into memory at slot memSlot.
    p_d->getDs().seekg(bNum * BLOCK_BYTES, ios_base::beg);
    p_d->getDs().read(reinterpret_cast<char *>(p_m->getPage(memSlot)), 
                      BLOCK_BYTES);
  
    // do our bookkeeping: in with the new
    blksInMem.set(bNum);
    blkLocsInMem[bNum] = memSlot;

    PgTabEntry temp(bNum, memSlot, getCurTime()); 
    p_m->takeAvlMemSlt(memSlot);

    pT.doPushHeap(std::move(temp));

    assert(pT.checkHeap());
}

// Pre: page corresponding to bNum is in memory and has been updated with any
//      pre-existing changes.
// Updates the access time of the page table entry, writes a message to clog,
//    and heapifies the page table if necessary.
// Called by processRequest()
void MemMan::rdInSlot(bNum_t bNum)
{
    uint32_t mSlot = blkLocsInMem[bNum]; 
    uint32_t pTSlot = pT.getPgTabSlotFrmMemSlot(mSlot);

    assert(pTSlot != pT.heapSize);

    pT.updateATime(pTSlot); 
    clog << tabs(1) << "Reading from page " << bNum << " in memory slot " 
         << mSlot << " at time " 
         << (pT.getPgTabEntry(pTSlot)).accTime << '\n';

    if (!pT.isLeaf(pTSlot))
        pT.heapify();
}

// Append the Change cg to ChangeLog.
// Pre: page corresponding to cg.blkNum is in Memory and has been updated with
//      any pre-existing changes.
// Updates the access time of the page table entry, writes a message to clog,
//    and heapifies the page table if necessary. 
// Adds current changes to change log.
// Called by processRequest()
void MemMan::wrtInSlot(const Change &cg)
{
    assert(cg.linesAltered());

    uint32_t mSlot = blkLocsInMem[cg.blkNum];
    uint32_t pTSlot = pT.getPgTabSlotFrmMemSlot(mSlot);

    bNum_t bNum = cg.blkNum;

    pT.updateATime(pTSlot);
    clog << tabs(1) << "Writing to page " << bNum << " in memory slot " 
         << mSlot << " at time "
         << (pT.getPgTabEntry(pTSlot)).accTime << '\n';

    p_cL->addToLog(cg);
        
    if (!pT.isLeaf(pTSlot))
        pT.heapify();
}

// Called by: evictThisPage()
bool MemMan::blkInPgTab(bNum_t bNum) const
{
    return blkLocsInMem.count(bNum) == 1;
}

// Called by: processRequest() if c.l. flag -v (verbose) is set
void MemMan::debugDisplay()
{
    clog << '\n';
    clog << "BLOCK   MSLOT   mslot   atime   ptslot" << '\n';
    clog << "=====   =====   =====   =====   ======" << '\n';
    for (bNum_t i = 0UL; i < NUM_DISK_BLOCKS; ++i)
        if (blksInMem.test(i)) {
            clog << setw(5) << right << i << "   ";
            clog << setw(5) << right << blkLocsInMem[i] << "   ";
            for (uint32_t j = 0; j < NUM_MEM_SLOTS; ++j) {
                if (pT.pgTab[j].blockNum == i) {
                    clog << setw(5) << right << pT.pgTab[j].memSlotIx << "   ";
                    clog << setw(5) << right << pT.pgTab[j].accTime << "    ";
                    clog << setw(6) << right << j << '\n';
                    break;
                }
            }
        }
    clog << '\n';
}
