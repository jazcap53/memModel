
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <limits>
#include <bitset>
#include <array>
#include <iomanip>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include "crashChk.h"
#include "journal.h"
#include "simDisk.h"
#include "crc.h"
#include "aJTypes.h"
#include "fileMan.h"

using std::fstream;
using std::ios_base;
using std::cerr;  using std::endl;  using std::clog;  using std::flush;
using std::clog;
using std::allocator;
using std::pair;
using std::numeric_limits;
using std::bitset;
using std::array;
using std::setw;

using namespace Disk;
using namespace MemModel;
using namespace CRC;
using namespace Setup;

///////////////////////////////
// PUBLIC MEMBER FUNCTIONS
///////////////////////////////

// Pre: SimDisk::SimDisk() has been called, so file fName exists
//      and is of the correct size. 
Journal::Journal(const std::string &fName, SimDisk *pd, ChangeLog *pcl,
                 Status *stt, CrashChk *pcck) : 
    js(fName, std::ios_base::in | std::ios_base::out | std::ios_base::binary), 
    p_d(pd), p_cL(pcl), p_stt(stt), p_cck(pcck)
{ 
    p_buf = bufAlloc.allocate(NUM_PGS_JRNL_BUF);

    if (p_cck->getLastStatus()[0] == 'C') {         // 'C': "Change log written"
        purgeJrnl(true, true);                // true: keepGoing, true: hadCrash
        p_stt->wrt("Last change log recovered");
    }
    init(); 
}

Journal::~Journal()
{ 
    if (js.is_open()) 
        js.close(); 
    bufAlloc.deallocate(p_buf, NUM_PGS_JRNL_BUF);
}

// Write the current change log to the journal file.
// Called by: MemMan::timedActs(), MemMan::~MemMan(), FileMan::addBlock()
void Journal::wrtCgLogToJrnl(ChangeLog &r_cgLog)
{
    if (r_cgLog.cgLineCt) {
        clog << "\n\tSaving change log:\n" << '\n';
        r_cgLog.print(clog);

        rdMetadata();

        js.seekp(metaPut, ios_base::beg);
        origPPos = js.tellp();
        // META_LEN is sizeof(metaGet) + sizeof(metaPut) + sizeof(metaSz) 
        assert(origPPos >= META_LEN);

        ttlBytes = 0UL;                        // cg bytes + start tag + end tag
        uint64_t cgBytes = 0L;
        uint64_t cgBytesPos = 0L;

        // write using 0 for value of cgBytes
        wrtCgsToJrnl(r_cgLog, &cgBytes, &cgBytesPos);
        wrtCgsSzToJrnl(&cgBytes, cgBytesPos);            // insert value cgBytes

        uint64_t newGPos = static_cast<uint64_t>(origPPos);
        uint64_t newPPos = static_cast<uint64_t>(js.tellp());
        uint64_t uTtlBytes = static_cast<uint64_t>(ttlBytes);
    
        wrtMetadata(&newGPos, &newPPos, &uTtlBytes);

        clog << '\t' << tabs(0) << "Change log written to journal at time " 
             << getCurTime() << '\n';
        r_cgLog.cgLineCt = 0;
        p_stt->wrt("Change log written");
    }
}

// Write contents of latest journal to disk_file.
//
// Read the latest journal from jrnl_file. Read pages into the buffer in 
// ascending order. Apply the changes to pages. Write the altered pages back 
// to disk in descending order.
//
// Called by: MemMan::MemMan(), MemMan::~MemMan(), MemMan::timedActs().
//    FileMan::addBlock()
void Journal::purgeJrnl(bool keepGoing, bool hadCrash)
{
    clog << tabs(1, true) << "Purging journal";
    if (hadCrash)
        clog << " (after crash)";
    clog << '\n';

    if (blksInJrnl.none() && !hadCrash)
        clog << "\tJournal is empty: nothing to purge\n\n";
    else {
        ChangeLog jCgLog;

        rdLastJrnl(jCgLog);

        uint32_t ctr = 0;       // number of PgPr's constructed in buffer *p_buf
        bNum_t prevBlkNum = numeric_limits<bNum_t>::max();
        bNum_t currBlkNum = numeric_limits<bNum_t>::max();
        Page pg;

        // Read, apply, write
        rdAndWrtBack(jCgLog, p_buf, &ctr, &prevBlkNum, &currBlkNum, pg);

        // Read, apply, write last page
        Change cg = jCgLog.theLog[currBlkNum].back();
        rAndWBLast(cg, p_buf, &ctr, currBlkNum, pg);

        // all objects in buffer have now been destroyed
        assert(ctr == 0);

        blksInJrnl.reset();
        // cgLog should only be cleared when the journal is purged
        p_cL->theLog.clear();
    }

    p_stt->wrt(keepGoing ? "Purged journal" : "Finishing");
}

// Copy changed lines from Change object into Page object.
// Called by: rdAndWrtBack(), rAndWBLast(), MemMan::updatePgInMem()
void Journal::wrtCgToPg(Change &cg, Page &pg) const
{
    cg.arrNext = 0UL;

    lNum_t linNum = getNextLinNum(cg);
    while (linNum != 0xFF) {
        Line temp = cg.newData.front();
        cg.newData.pop_front();
        memcpy(&pg.dat[linNum * BYTES_PER_LINE], &temp, BYTES_PER_LINE);
        linNum = getNextLinNum(cg);
    }
}

// Called by: FileMan::remvBlock()
bool Journal::isInJrnl(bNum_t bNum) const
{
    return blksInJrnl.test(bNum);
}

// Make sure that a newly added block is clean, and that it will not be
//    overwritten by defunct changes.
// Called by: FileMan::addBlock()
void Journal::doWipeRoutine(bNum_t bNum, FileSys::FileMan *p_fM)
{
    if (wipers.isDirty(bNum) || wipers.isRipe()) {
        p_fM->doStoreInodes();
        p_fM->doStoreFreeList();
        clog << '\n' << tabs(1) << "Saving change log and purging "
             << "journal before adding new block\n";
        wrtCgLogToJrnl(*p_cL);
        purgeJrnl();                                             // to main disk
        wipers.clearArray();
    }
}

////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////

// Called by: Journal()
void Journal::init()
{
    // rdPt, wrtPt, and bytesStored are the META fields
    int64_t rdPt = -1L;
    int64_t wrtPt = 24;
    int64_t bytesStored = 0;    // value does not include size of these 3 fields

    js.write(reinterpret_cast<char *>(&rdPt), sizeof(rdPt));
    js.write(reinterpret_cast<char *>(&wrtPt), sizeof(wrtPt));
    js.write(reinterpret_cast<char *>(&bytesStored), sizeof(bytesStored));
}

// Writes field to circular buffer.
// Avoids overwriting the META_LEN bytes at beginning of the file. 
// Adds bytes written to ttlBytes unless parameter doCt is false
// Called by: wrtCgLogToJrnl()
void Journal::wrtField(const char *cp_dat, uint32_t datLen, bool doCt)
{
    uint64_t pPos = js.tellp();
    constexpr uint64_t bufSz = static_cast<uint64_t>(JRNL_SIZE);
    uint64_t endPt = pPos + datLen;
    
    if (endPt > bufSz) {
        uint64_t over = endPt - bufSz;
        uint64_t under = datLen - over;
        js.write(cp_dat, under);
        if (doCt)
            ttlBytes += under;
        js.seekp(META_LEN, ios_base::beg);
        js.write(cp_dat + under, over);
        if (doCt)
            ttlBytes += over;
    }
    else if (endPt == bufSz) {
        js.write(cp_dat, datLen);
        if (doCt)
            ttlBytes += datLen;
        js.seekp(META_LEN, ios_base::beg);
    }
    else {
        js.write(cp_dat, datLen);
        if (doCt)
            ttlBytes += datLen;
    }
}

// Advances 'put' position in circular buffer, without writing
// anything. Avoids overwriting the META_LEN bytes at beginning 
// of the file. 
// Called by: wrtCgLogToJrnl()
void Journal::advanceStrm(int64_t len)
{
    int64_t newPos = js.tellp() + len;
    if (newPos >= JRNL_SIZE) {
        newPos -= JRNL_SIZE;
        newPos += META_LEN;
    }
    js.seekp(newPos, ios_base::beg);    
}

// Write each field: add its size to ttlBytes.
// Called by: wrtCgLogToJrnl()
void Journal::wrtCgsToJrnl(ChangeLog &r_cgLog, uint64_t *p_cgBytes, 
                           uint64_t *p_cgBytesPos)
{
    wrtField(reinterpret_cast<const char *>(&START_TAG), sizeof(START_TAG), 
             true);

    *p_cgBytesPos = js.tellp();
    // cgBytes is zeroes for now; acts as a placeholder
    wrtField(reinterpret_cast<const char *>(p_cgBytes), sizeof(*p_cgBytes), 
             true);

    // theLog: map<block number, list<Change>>
    for (auto prCgLst : r_cgLog.theLog)   // prCgLst: pair<blkNum, list<Change>>
        for (auto cg : prCgLst.second) {    
            wrtField(reinterpret_cast<const char *>(&cg.blkNum), 
                     sizeof(cg.blkNum), true);
            blksInJrnl.set(cg.blkNum);                      // set bit in bitset
            wrtField(reinterpret_cast<const char *>(&cg.timeStamp), 
                     sizeof(cg.timeStamp), true);

            for (const auto &s : cg.selectors)
                wrtField(reinterpret_cast<const char *>(&s), sizeof(select_t), 
                         true);

            for (const auto &d : cg.newData) {
                wrtField(reinterpret_cast<const char *>(&d), BYTES_PER_LINE,
                         true);
            }
        }

    wrtField(reinterpret_cast<const char *>(&END_TAG), sizeof(END_TAG),
             true);

	doTest1();                        // test 1: have we written ttlBytes bytes?
}

//Called by: wrtCgLogToJrnl()
void Journal::wrtCgsSzToJrnl(uint64_t *p_cgBytes, uint64_t cgBytesPos)
{
    assert(sizeof(END_TAG) + sizeof(START_TAG) <= ttlBytes);
	*p_cgBytes = ttlBytes - sizeof(END_TAG) - sizeof(START_TAG); 

    js.seekp(cgBytesPos, ios_base::beg);
	// false: don't add these bytes to ttlBytes
    wrtField(reinterpret_cast<const char *>(p_cgBytes), sizeof(*p_cgBytes), 
             false);
    
    // advance to end of current journal
    advanceStrm(*p_cgBytes - sizeof(*p_cgBytes) + sizeof(END_TAG)); 
    
    // was write of *p_cgBytes ok?
    // tellp() returns a signed 128-bit value on my (64-bit) system
    assert(finalPPos == static_cast<uint64_t>(js.tellp()));
}

// Test that wrtCgLogToJrnl() wrote the correct number of bytes.
// Called by: wrtCgLogToJrnl()
void Journal::doTest1()
{
    bool ok = false;
    finalPPos = js.tellp();

    assert(ttlBytes > 0 && ttlBytes < JRNL_SIZE - META_LEN);

    if (origPPos < finalPPos) 
        ok = (finalPPos - origPPos == ttlBytes);
    else if (finalPPos < origPPos) 
        ok = (origPPos + ttlBytes + META_LEN - JRNL_SIZE == finalPPos);

    assert(ok);
}

// Called by: wrtCgLogToJrnl(), rdLastJrnl()
void Journal::rdMetadata()
{
    js.seekg(0, ios_base::beg);
    js.read(reinterpret_cast<char *>(&metaGet), sizeof(metaGet));
    js.read(reinterpret_cast<char *>(&metaPut), sizeof(metaPut));
    js.read(reinterpret_cast<char *>(&metaSz), sizeof(metaSz));
}

// Called by: wrtCgLogToJrnl()
void Journal::wrtMetadata(uint64_t *p_newGPos, uint64_t *p_newPPos, 
                          uint64_t *p_uTtlBytes)
{
    js.seekp(0, ios_base::beg);
    js.write(reinterpret_cast<char *>(p_newGPos), sizeof(*p_newGPos));
    js.write(reinterpret_cast<char *>(p_newPPos), sizeof(*p_newPPos));
    js.write(reinterpret_cast<char *>(p_uTtlBytes), sizeof(*p_uTtlBytes));
}

// Read changes from log, read pages from disk into buffer, 
//   apply changes to pages.
// Empty buffer when full.
// Called by: purgeJrnl()
void Journal::rdAndWrtBack(const ChangeLog &jCgLog, PgPr * const p_buf, 
                           uint32_t *p_ctr, bNum_t *p_prvBlkNum,
                           bNum_t *p_curBlkNum, Page &pg)
{
    for (auto prCgLst : jCgLog.theLog) {  // prCgLst: pair<blkNum, list<Change>>
        for (auto cg : prCgLst.second) {
            *p_curBlkNum = cg.blkNum;             // get number of current block
            if (*p_curBlkNum != *p_prvBlkNum) {           // if it's a new block
                // if there's a previous block
                if (*p_prvBlkNum != numeric_limits<bNum_t>::max()) {
                    if (*p_ctr == NUM_PGS_JRNL_BUF)         // if buffer is full
                        emptyPurgeJrnlBuf(p_buf, p_ctr);     // send cgs to disk

                    // construct a pgPr in buffer
                    bufAlloc.construct(p_buf + *p_ctr, *p_prvBlkNum, 
                                       std::move(pg));
                    ++*p_ctr;
                }

                // read unaltered block from disk into pg
                pg = Page();       // assign to pg that may have been moved from
                p_d->getDs().seekg(*p_curBlkNum * BLOCK_BYTES, ios_base::beg);
                p_d->getDs().read(reinterpret_cast<char *>(pg.dat), 
                                  BLOCK_BYTES);
            }
            *p_prvBlkNum = *p_curBlkNum;
        
            wrtCgToPg(cg, pg);                        // write this change to pg
        }
    }
}

// Process the last cgLog entry, empty buffer
// Called by: purgeJrnl()
void Journal::rAndWBLast(Change &cg, PgPr * const p_buf, uint32_t *p_ctr, 
                         bNum_t curBlkNum, Page &pg)
{
    // if buffer is full
    if (*p_ctr == NUM_PGS_JRNL_BUF)
        emptyPurgeJrnlBuf(p_buf, p_ctr);
    
    bufAlloc.construct(p_buf + *p_ctr, curBlkNum, std::move(pg));
    ++*p_ctr;

    wrtCgToPg(cg, pg);
    
    // whether buffer is full or not
    emptyPurgeJrnlBuf(p_buf, p_ctr, true);    
}

// Read last journal entry into r_jCgLog
// Called by: purgeJrnl()
void Journal::rdLastJrnl(ChangeLog &r_jCgLog)
{
    rdMetadata();

    js.seekg(metaGet, ios_base::beg);
    int64_t origGPos = js.tellg();

    assert(origGPos >= META_LEN);

    ttlBytes = 0UL;                // cg bytes + start tag bytes + end tag bytes
    uint64_t cgBytes = 0L;
	uint64_t ckStartTag = 0;
	uint64_t ckEndTag = 0;

	rdJrnl(r_jCgLog, &cgBytes, &ckStartTag, &ckEndTag);

    assert(ckStartTag == START_TAG);
    assert(ckEndTag == END_TAG);
    assert(ttlBytes == cgBytes + sizeof(START_TAG) + sizeof(END_TAG));
}

// Read a journal entry into a ChangeLog
// Called by: rdLastJrnl()
void Journal::rdJrnl(ChangeLog &r_jCgLog, uint64_t *p_cgBytes,
					 uint64_t *p_ckStartTag, uint64_t *p_ckEndTag)
{
	ttlBytes = 0;

    rdField(reinterpret_cast<char *>(p_ckStartTag), sizeof(*p_ckStartTag));
    rdField(reinterpret_cast<char *>(p_cgBytes), sizeof(*p_cgBytes));
    
    bNum_t bNum = static_cast<bNum_t>(0UL);
    while (ttlBytes < *p_cgBytes + sizeof(START_TAG)) {
        rdField(reinterpret_cast<char *>(&bNum), sizeof(bNum));

        Change cg(bNum, false);              // false: create w/ empty Selectors

        rdField(reinterpret_cast<char *>(&cg.timeStamp), sizeof(cg.timeStamp));

        uint64_t numDataLines = getNumDataLines(cg);

        for (uint64_t i = 0; i != numDataLines; ++i) {
            Line aLine;
            rdField(reinterpret_cast<char *>(&aLine), BYTES_PER_LINE);
            cg.newData.push_back(aLine);
        }

        r_jCgLog.addToLog(cg);
    }

    rdField(reinterpret_cast<char *>(p_ckEndTag), sizeof(*p_ckEndTag));

    // This assert will fail if size of lNum_t is set to > 1 byte
    assert(*p_cgBytes + sizeof(START_TAG) + sizeof(END_TAG) == ttlBytes);
}

// Read selectors from journal file until the last byte of a selector == 0xFF,
//   indicating no more data. Then search backwards from the end of the selector
//   until the first non-0xFF byte is encountered. Return the total of non-0xFF
//   bytes.
// rdField() adds the number of bytes read to the ttlBytes data member.
// Called by: rdJrnl()
uint64_t Journal::getNumDataLines(Change &r_Cg)
{
    uint64_t numDataLines = 0UL;
    select_t tempSel = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    uint64_t setback = 1U;
    const uint64_t szUL = sizeof(tempSel);
    const uint32_t sz = sizeof(tempSel);

    rdField(reinterpret_cast<char *>(&tempSel), sz);
    r_Cg.selectors.push_back(tempSel);
    numDataLines += szUL - 1;

    while (tempSel[szUL - setback] != 0xFF) {
        rdField(reinterpret_cast<char *>(&tempSel), sz);
        r_Cg.selectors.push_back(tempSel);
        numDataLines += szUL;
    }
    ++setback;
    while (setback <= szUL && tempSel[szUL - setback] == 0xFF) {
        ++setback;
        --numDataLines;
    }
    return numDataLines;
}

// Read a field from the journal file. If the field would reach or pass the end-
//   of-file, ensure that reading will continue at the first byte past the 
//   metadata at the beginning of the file.
// Track the number of bytes read in ttlBytes.
// Called by: rdLastJrnl()
void Journal::rdField(char *p_dat, uint32_t datLen)
{
    uint64_t gPos = js.tellg();
    constexpr uint64_t bufSz = static_cast<uint64_t>(JRNL_SIZE);
    uint64_t endPt = gPos + datLen;
    
    if (endPt > bufSz) {
        uint64_t over = endPt - bufSz;
        uint64_t under = datLen - over;
        js.read(p_dat, under);        
        ttlBytes += under;
        js.seekg(META_LEN, ios_base::beg);                  // skip the metadata
        js.read(p_dat + under, over);
        ttlBytes += over;
    }
    else if (endPt == bufSz) {
        js.read(p_dat, datLen);
        ttlBytes += datLen;
        js.seekg(META_LEN, ios_base::beg);
    }
    else {
        js.read(p_dat, datLen);
        ttlBytes += datLen;
    } 
}

// Write buffer to disk (in reverse page order)
// Overwrite dirty pages that are no longer in use.
// Destroy pages in buffer.
// Called by: rdAndWrtBack(), rAndWBLast()
bool Journal::emptyPurgeJrnlBuf(PgPr * const p_pgPr, uint32_t *p_ctr,
                                bool isEnd)
{
    SimSector temp;                     // Create an empty block (with CRC code)
    p_d->doCreateBlock(temp, BLOCK_BYTES); 

    while (*p_ctr) {   // *p_ctr: number of PgPr's constructed in buffer *p_pgPr
        --(*p_ctr);           // we only want to decrement if the test succeeds!
        pair<bNum_t, Page> *cursor = p_pgPr + *p_ctr;
        crcCheckPg(cursor);
        p_d->getDs().seekp(cursor->first * BLOCK_BYTES, ios_base::beg);

        if (wipers.isDirty(cursor->first)) { // if cursor->first is on wipe list
            p_d->getDs().write(reinterpret_cast<char *>(&temp.sect), 
                               BLOCK_BYTES);
            clog << tabs(3, true) << "Overwriting dirty block " << cursor->first
                 << '\n';
        }
        else {
            p_d->getDs().write(reinterpret_cast<char *>(&(cursor->second)), 
                               BLOCK_BYTES);
            clog << tabs(3, true) << "Writing page " << setw(3) 
                 << cursor->first << " to disk\n";
        }
        bufAlloc.destroy(cursor);
    }
    if (!isEnd)
        clog << '\n';

    bool okVal = true;
    if (!p_d->getDs()) {
        cerr << "ERROR: Error writing to " << p_d->getDFileName() << " at " 
             << __FILE__ << ", " << __LINE__ << '\n';
        okVal = false;
    }

    return okVal;
}

// Perform CRC on page number p_pr->first
// Called by: emptyPurgeJrnlBuf()
void Journal::crcCheckPg(PgPr * const p_pr) const
{
    unsigned char *p_ucDat = p_pr->second.dat;
    
    BoostCRC::wrtBytesBigE(0x00000000, p_ucDat + BYTES_PER_PAGE - CRC_BYTES,
                           CRC_BYTES);
    uint32_t code = BoostCRC::getCode(p_ucDat, BYTES_PER_PAGE);
    BoostCRC::wrtBytesBigE(code, p_ucDat + BYTES_PER_PAGE - CRC_BYTES,
                           CRC_BYTES);
    uint32_t code2 = BoostCRC::getCode(p_ucDat, BYTES_PER_PAGE);

    assert(code2 == 0U);
}

// Called by: wrtCgToPg()
lNum_t Journal::getNextLinNum(Change &cg) const
{
    lNum_t linNum = 0xFF;

    assert(!cg.selectors.empty());

    linNum = cg.selectors.front()[cg.arrNext++];

    if (cg.arrNext == sz) {
        cg.selectors.pop_front();
        cg.arrNext = 0UL;
    }

    return linNum;
}
