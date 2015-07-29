#ifndef AAJ_THE_JOURNAL_CLASS_H
#define AAJ_THE_JOURNAL_CLASS_H

#include <fstream>
#include <memory>
#include <utility>
#include <bitset>
#include <string>
#include "crashChk.h"
#include "myMemory.h"
#include "change.h"
#include "wipeList.h"
#include "aJTypes.h"
#include "aJUtils.h"

namespace FileSys {
    class FileMan;
}

namespace Disk {
    using namespace MemModel;
    using namespace Setup;

    class SimDisk;
    class Status;

    /*****************************
      Journal simulates a disk journal. It contains a stream attached to a file
      that is maintained as a circular buffer. 
      
      The first 3 8-byte fields of the file hold the get point (beginning),
      put point (one byte past the end), and size in bytes of the most recent
      complete journal entry. These are the META fields. 
    
      They are followed by a series of journal entries arranged in a circular
      fashion.
    
      The structure of a journal entry may be represented as follows:
    
                              A                      B             B
                             //                     //             \\
       ||    8     |    8    ||    4    |     8     ||      8      ||
       ||          |         ||:        |           ||:           :||
       || startTag | cgBytes ||  blkNum | timeStamp ||  selectors  ||
                             \\                     \\             //
 
        C        C    A
       //        \\   \\
       ||   64   ||   ||   8    ||
       ||:      :||  :||        ||     The sizes given are in bytes.
       ||  data  ||   || endTag ||
       \\        //   //
    *****************************/

    class Journal {
        const uint64_t START_TAG = 17406841880640449871UL;     // 'random' value
        const uint64_t END_TAG = 4205560943366639022UL;              // likewise
        const uint32_t META_LEN = 24;                         // 3 8-byte fields
        // Journal holds only changes; journal buffer holds full pages
        const uint32_t NUM_PGS_JRNL_BUF = 16;
        typedef std::pair<bNum_t, Page> PgPr;           // <Page number, Page>

    public:
        Journal(const std::string &fName, SimDisk *pd, ChangeLog *pcl,
                Status *stt, CrashChk *pcck);
        Journal(const Journal &) = delete;
        ~Journal();

        void wrtCgLogToJrnl(ChangeLog &r_cgLog);
        void purgeJrnl(bool keepGoing = true, bool hadCrash = false);
        void wrtCgToPg(Change &cg, Page &pg) const;
        uint64_t getLastJrnlPurgeTime() const { return lastJrnlPurgeTime; }
        void setLastJrnlPurgeTime(uint64_t tm) { lastJrnlPurgeTime = tm; }
        bool isInJrnl(bNum_t bNum) const;
        void doWipeRoutine(bNum_t bNum, FileSys::FileMan *p_fM);
        void setWiperDirty(bNum_t bNum) { wipers.setDirty(bNum); }
        
    private:
        void init();
        void wrtField(const char *cp_dat, uint32_t datLen, bool doCt);
        void advanceStrm(int64_t len);
		void wrtCgsToJrnl(ChangeLog &r_cgLog, uint64_t *p_cgBytes, 
                              uint64_t *p_cgBytesPos);
		void wrtCgsSzToJrnl(uint64_t *p_cgBytes, uint64_t cgBytesPos);
        void doTest1();
        void rdMetadata();
        void wrtMetadata(uint64_t *p_newGPos, uint64_t *p_newPPos, 
                         uint64_t *p_uTtlBytes);
        void rdAndWrtBack(const ChangeLog &jCgLog, PgPr * const p_buf, 
                          uint32_t *p_ctr, bNum_t *p_prvBlkNum,
                          bNum_t *p_curBlkNum, Page &pg);
        void rAndWBLast(Change &Cg, PgPr * const p_buf, uint32_t *p_ctr, 
                        bNum_t currBlkNum, Page &pg);
        void rdLastJrnl(ChangeLog &r_cgLog);
        void rdJrnl(ChangeLog &r_jCgLog, uint64_t *p_cgBytes,
                    uint64_t *p_ckStartTag, uint64_t *p_ckEndTag);
        uint64_t getNumDataLines(Change &r_Cg);
        void rdField(char *p_dat, uint32_t datLen);
        bool emptyPurgeJrnlBuf(PgPr * const p_pgPr, uint32_t *p_ctr,
                               bool isEnd = false);
        void crcCheckPg(PgPr * const p_pr) const;
        lNum_t getNextLinNum(Change &cg) const;

        std::fstream js;
        std::allocator<PgPr> bufAlloc;
        PgPr *p_buf;
        uint64_t metaGet = 0UL;
        uint64_t metaPut = 0UL;
        uint64_t metaSz = 0UL;
        uint64_t ttlBytes = 0UL;
        uint64_t origPPos = 0UL;
        uint64_t finalPPos = 0UL;
        uint32_t sz = sizeof(select_t);
        uint64_t szUL = sizeof(select_t);
        std::bitset<bNum_tConst::NUM_DISK_BLOCKS> blksInJrnl = 0UL;
        SimDisk *p_d;
        ChangeLog *p_cL;
        Status *p_stt;
        CrashChk *p_cck;
        uint64_t lastJrnlPurgeTime = 0UL;
        Tabber tabs;
        WipeList wipers;
    };

}

#endif
