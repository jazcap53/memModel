#include <limits>
#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include "client.h"
#include "change.h"
#include "aJTypes.h"
#include "aJUtils.h"

using namespace User;

using std::clog;  using std::endl;  using std::flush;
using std::ostringstream;
using std::string;
using std::vector;
using std::find_if;  using std::min;
using std::uniform_int_distribution;

///////////////////////////////
// PUBLIC MEMBER FUNCTIONS
///////////////////////////////

Client::Client(uint32_t id, FileMan *pfm, Driver *pdrvr):
    myId(id), p_fM(pfm), p_drvr(pdrvr)
{
    init();
}

void Client::init()
{
#ifdef AWAIT_CTRL_C
    numRequests = 200 * RUN_FACTOR * PAGES_PER_JRNL;      // run for a long time
#else
    numRequests = 
        p_drvr->getLongRun() ? RUN_FACTOR * PAGES_PER_JRNL : SHORT_RUN;
#endif
    if (p_drvr->getTest()) {  // seed each client with a unique repeatable value
        uint64_t initialSeed = p_drvr->getTheSeed();
        kRE.seed(initialSeed + getMyId() * initialSeed);
    }
    else                    // seed each client with a unique pseudorandom value
        kRE.seed(system_clock::to_time_t(system_clock::now()));

    srand(p_drvr->getTheSeed());

    // dist_* typedef'd as uniform_int_distribution<type>
    dist_bNum::param_type pt_whichBlock{loPage, hiPage};
    dist_i32::param_type pt_rdOrWrt{loChooser, hiChooser};
    dist_u64::param_type pt_delay{minDelay, maxDelay};
    dist_i32::param_type pt_actions{0, actions};
    dist_inNum::param_type pt_whichInode{0, hiInode};

    uID_whichBlock.param(pt_whichBlock);
    uID_rdOrWrt.param(pt_rdOrWrt);
    uID_delay.param(pt_delay);
    uID_actions.param(pt_actions);
    uID_whichInode.param(pt_whichInode);
}

// Generates requests to read or write random disk blocks and
//   sends these requests to the file manager.
// Called by: main()
void Client::makeRequests()
{
    for (int32_t i = 0; i != numRequests; ++i) {
        rndDelay();
        int32_t act = uID_actions(kRE);           // act: trial-and-error values

        if (act < 5) {   // create a file if possible; else delete a random file
            createOrDelete();
        }
        else if (act < 8) {  // delete random file if possible; else create file
            deleteOrCreate();
        }
        else if (act < 20) {      // pick a random file; add a block if possible
            addRndBlock(&i);
        }        
        else if (act < 23) {  // pick a rnd file; remove a rnd block if possible
            remvRndBlock(&i);
        }
        else {
            makeRWRequest(&i);
        }
    }

    clog.flush();
}

///////////////////////////////
// PRIVATE MEMBER FUNCTIONS
///////////////////////////////

// If we have room, create a file
// Else, delete a file
// Called by: makeRequests()
void Client::createOrDelete()
{
    if (reqCountFiles() < NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK - 1)
        reqCreateFile();
    else
        reqDeleteFile(uID_whichInode(kRE));
}

// If we have files, delete a file
// Else, create a file
// Called by: makeRequests()
void Client::deleteOrCreate()
{
    inNum_t tgt = rndFileNum();
    if (tgt != SENTINEL_INUM)
        reqDeleteFile(tgt);
    else
        reqCreateFile();    
}

// For a random file, if the file has room, add the first
//    available block from the free list
// Called by: makeRequests()
void Client::addRndBlock(int32_t *p_i)
{
    inNum_t tgt = rndFileNum();
    if (tgt != SENTINEL_INUM) {
        if (reqCountBlocks(tgt) < CT_INODE_BNUMS - 1)
            bNum_t added = reqAddBlock(tgt);
        else
            --*p_i;
    }
    else 
        --*p_i;
}

// For a random file, if it has blocks,
//    remove a random block
// Called by: makeRequests()
void Client::remvRndBlock(int32_t *p_i)
{
    inNum_t tgtNdNum = rndFileNum();
    if (tgtNdNum != SENTINEL_INUM) {
        bNum_t tgtBlkNum = rndBlkNum(tgtNdNum);
        if (tgtBlkNum != SENTINEL_BNUM)
            reqRemvBlock(tgtNdNum, tgtBlkNum);
        else
            --*p_i;
    }
    else
        --*p_i;
}

// Set up a request to read or write a block, and submit
//    the request to file manager
// Called by: makeRequests()
void Client::makeRWRequest(int32_t *p_i)
{
    inNum_t tgtNdNum = rndFileNum();
    if (tgtNdNum != SENTINEL_INUM) {
        bNum_t tgtBlkNum = rndBlkNum(tgtNdNum);
        if (tgtBlkNum != SENTINEL_BNUM) {
            int32_t rdOrWrt = uID_rdOrWrt(kRE);
            bool doWrt = rdOrWrt >= rdPct;
            Change cg(tgtBlkNum);
            if (doWrt)
                setUpCgs(cg); 
            reqSubmitRequest(doWrt, myId, tgtNdNum, cg);
        }
        else
            --*p_i;
    }
    else
        --*p_i;
}

// Creates a file
// On success, returns the file's inode number
// On fail, returns SENTINEL_INUM
// Called by: createOrDelete(), deleteOrCreate()
inNum_t Client::reqCreateFile()
{
    return p_fM->createFile();
}

// Deletes a file
// Returns: true on successful delete
// Called by: createOrDelete(), deleteOrCreate()
bool Client::reqDeleteFile(inNum_t iNum)
{
    return p_fM->deleteFile(myId, iNum);
}

// Returns: the number of files being managed by this system
// Called by: createOrDelete()
inNum_t Client::reqCountFiles() const
{
    return p_fM->countFiles();
}

// Returns: the number of blocks assigned to iNum
// Called by: addRndBlock()
bNum_t Client::reqCountBlocks(inNum_t iNum) const
{
    return p_fM->countBlocks(iNum);
}

// Returns: whether a file with inode iNum exists
// Called by: rndFileNum()
bool Client::reqFileExists(inNum_t iNum) const
{
    return p_fM->fileExists(iNum);
}

// On success: returns the number of block newly added to iNum
// On fail: returns SENTINEL_BNUM
// Called by: addRndBlock()
bNum_t Client::reqAddBlock(inNum_t iNum)
{
    return p_fM->addBlock(myId, iNum);
}

// Returns whether block bNum was successfuly removed from iNum
// Called by: makeRequests()
bool Client::reqRemvBlock(inNum_t iNum, bNum_t bNum)
{
    return p_fM->remvBlock(myId, iNum, bNum);
}

// Returns the inode associated with iNum
// Called by: rndBlkNum()
const Inode &Client::reqGetInode(inNum_t iNum) const
{
    return p_fM->getInode(iNum);
}

// Request read of a block or write of changes to a block
// Called by: makeRequests()
void Client::reqSubmitRequest(bool doWrt, uint32_t cliId, inNum_t iNum,
                              const MemModel::Change &cg)
{
    p_fM->submitRequest(doWrt, cliId, iNum, cg);
}

// If there are files:
//    return one of their iNums
// Else:
//    return SENTINEL_INUM
// Called by: makeRequests()
inNum_t Client::rndFileNum() const
{
    inNum_t found = SENTINEL_INUM;

    inNum_t fileCt = reqCountFiles();
    if (fileCt) {
        inNum_t tgt = rand() % 
            (NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK);
        while (!reqFileExists(tgt))
            tgt = rand() % 
                (NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK);
        found = tgt;
    }

    return found;
}

// If file tgtNdNum exists and has blocks:
//     return the bNum of one of them
// Else:
//     return SENTINEL_BNUM
// Called by: makeRequests()
bNum_t Client::rndBlkNum(inNum_t tgtNdNum) const
{
    bNum_t found = SENTINEL_BNUM;

    if (reqCountBlocks(tgtNdNum)) {
        const Inode &tmp = reqGetInode(tgtNdNum);
        uint32_t tgtBlkIx = rand() % CT_INODE_BNUMS;
        bNum_t tgtBlk = tmp.bNums[tgtBlkIx];
        while (tgtBlk == SENTINEL_INUM) {
            tgtBlkIx = rand() % CT_INODE_BNUMS;            // pick another block
            tgtBlk = tmp.bNums[tgtBlkIx];
        }
        found = tgtBlk;
    }

    return found;
}

// Specify which lines in a page should be altered
// If c.l. arg test == true, the first line of each altered page is set to
// its block number.
// Called by: makeRequests()            
void Client::setUpCgs(Change &cg)
{
    const uint32_t maxLinesChanged = 15U;       // convenient arbitrary constant

    dist_lNum uID_whichLineIfTest(1U, LINES_PER_PAGE - 1U); 
    dist_lNum uID_whichLine(0U, LINES_PER_PAGE - 1U); 
    dist_u32 uID_numCgs(1U, maxLinesChanged);

    uint32_t numCgs = uID_numCgs(kRE);
    bNum_t bNum = cg.blkNum;

    bool test = p_drvr->getTest();
    for (uint32_t i = 0U; i != numCgs; ++i) {
        ostringstream oss;
        Line lin = {};      // Line is std::array<unsigned char, BYTES_PER_LINE>
        lNum_t linNum = 0U;
        string s;
        if (test && !i) {
            oss << "Block " << bNum << '\n';
        }
        else {
            if (test)
                linNum = uID_whichLineIfTest(kRE);
            else
                linNum = uID_whichLine(kRE);                
            oss << "Line " << static_cast<uint32_t>(linNum) << '\n';
        }
        s = oss.str();
        assert(s.size() <= BYTES_PER_LINE);

        linCpy(lin, s);
        cg.addLine(bNum, linNum, lin);
    }
}

// Copy data string into bytes [0..62] of Line ln. Byte 63 holds index of first
// byte of ln *NOT* being used, i.e. 0x00 for empty ln, 0x3F for full ln.
// Called by: setUpCgs()
void Client::linCpy(Line &ln, const string &s) const
{
    uint32_t sz = s.size();
    assert(sz < BYTES_PER_LINE);

    unsigned char marker = sz;
    for (uint32_t i = 0; i < BYTES_PER_LINE - 1; ++i)
        ln[i] = (i < sz ? static_cast<unsigned char>(s[i]) : 0U);

    ln[BYTES_PER_LINE - 1] = marker;
}

void Client::rndDelay()
{
    uint64_t delay = uID_delay(kRE);
    uint64_t begin = getCurTime();
    while (getCurTime() - begin < delay)
        ;            
}

// Not in use.
// Removes an entry from myOpenFiles vector by:
//   1) locating that entry
//   2) swapping the entry to the end of myOpenFiles
//   3) calling pop_back() on myOpenFiles
// Called by: reqDeleteFile()
bool Client::remvFrmMyOpenFiles(inNum_t iNum)
{
    bool removed = false;

    auto f = [iNum](const vector<inNum_t> &v) { return v[0] == iNum; };
    auto iter = find_if(myOpenFiles.begin(), myOpenFiles.end(), f);
    if (iter != myOpenFiles.end()) {
        auto sz = myOpenFiles.size();
        if (sz > 1) {
            swap(*iter, myOpenFiles[sz - 1]);
            myOpenFiles.pop_back();
        }
        removed = true;
    }
    
    return removed;
}
