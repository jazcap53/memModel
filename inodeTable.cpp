#include <cstdint>
#include <ios>
#include <iostream>
#include <chrono>
#include <limits>
#include <cassert>
#include <fstream>
#include <functional>
#include <forward_list>
#include <string>
#include "inodeTable.h"

using namespace std::chrono;
using namespace FileSys;
using namespace std::placeholders;

using std::ios_base;
using std::cerr;  using std::endl;  using std::clog;
using std::numeric_limits;
using std::fstream;
using std::ref;  using std::bind;
using std::forward_list;
using std::string;

//////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
//////////////////////////////////////

// Pre: inode table file node_file exists
// Called by: FileMan()
InodeTable::InodeTable(string nfn): ns(nfn, ios_base::in | ios_base::out 
                                       | ios_base::binary), fileName(nfn)
{
    assert(ns);

    loadTbl();
}

// Called by: ~FileMan()
InodeTable::~InodeTable()
{
    storeTbl();

    if (ns.is_open())
        ns.close();
}

Inode &InodeTable::refTblNode(inNum_t iNum)
{
    uint32_t blkNum = iNum / INODES_PER_BLOCK;
    uint32_t blkIx = iNum % INODES_PER_BLOCK;
    
    return tbl[blkNum][blkIx];
}

const Inode &InodeTable::refTblNode(inNum_t iNum) const
{
    uint32_t blkNum = iNum / INODES_PER_BLOCK;
    uint32_t blkIx = iNum % INODES_PER_BLOCK;
    
    return tbl[blkNum][blkIx];
}

// Return the first available inode number, if any
// Else return SENTINEL_INUM
// Called by:FileMan::createFile()
inNum_t InodeTable::assignInN()
{
    inNum_t ret = SENTINEL_INUM;
    const inNum_t MAX_INUM = NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK;

    for (inNum_t ix = 0U; ix != MAX_INUM; ++ix)
        if (avail.test(ix)) {
            ret = ix;
            avail.reset(ix);
            break;
        }

    if (ret != SENTINEL_INUM)
        refTblNode(ret).crTime = getCurTime(true);
    
    return ret;
}

// Called by: FileMan::deleteFile()
void InodeTable::releaseInN(inNum_t iNum)
{
    if (iNum == SENTINEL_INUM)
        return;

    Inode &r_tblNode = refTblNode(iNum);

    // clear the node
    for (auto &bN : r_tblNode.bNums)
        bN = SENTINEL_BNUM;
    r_tblNode.crTime = 0UL;
    for (auto &bN : r_tblNode.indirect)
        bN = SENTINEL_BNUM;

    avail.set(iNum);                              // mark the inode as available
}

// Called by: FileMan::fileExists(), remvBlock()
bool InodeTable::nodeInUse(inNum_t iNum) const
{   
    assert(iNum != SENTINEL_INUM);

    return !avail.test(iNum);
}

// Called by: FileMan::addBlock(), FileMan::remvBlock()
bool InodeTable::nodeLocked(inNum_t iNum) const
{
    assert(iNum != SENTINEL_INUM);

    return refTblNode(iNum).lkd != SENTINEL_INUM;
}

// Assign block blk to inode iNum if there is room in the inode
// We have already checked that the inode has not been locked by another client
// Called by: FileMan::addBlock()
bool InodeTable::assignBlkN(inNum_t iNum, bNum_t blk)
{
    assert(iNum != SENTINEL_INUM);
    assert(!avail.test(iNum)); // check we're not assigning block to unused node

    bool ret = false;

    for (auto &item : refTblNode(iNum).bNums)
        if (item == SENTINEL_BNUM) {   // item represents an open slot in inode 
            item = blk;
            ret = true;
            break;
        }

    return ret;
}

// Called by: releaseAllBlkN(), fileMan::remvBlock()
bool InodeTable::releaseBlkN(inNum_t iNum, bNum_t tgt)
{
    bool found = false;

    if (iNum != SENTINEL_INUM)
        for (auto &item: refTblNode(iNum).bNums) {
            if (item == tgt) {
                found = true;
                item = SENTINEL_BNUM;                       // release the block
                clog << tabs(2, true) << "Releasing block number " << tgt 
                     << " from inode " << iNum << '\n';

                break;
            }
        }

    return found;
}

void InodeTable::releaseAllBlkN(inNum_t iNum)
{
    if (iNum == SENTINEL_INUM)
        return;

    for (auto &item : refTblNode(iNum).bNums)                // item is a bNum_t
        if (item != SENTINEL_BNUM)
            releaseBlkN(iNum, item);
}

forward_list<bNum_t> InodeTable::listAllBlkN(inNum_t iNum)
{
    forward_list<bNum_t> tempList;

    if (iNum == SENTINEL_INUM)
        cerr << "InodeTable::listAllBlkN() called with SENTINEL_INUM" << endl;
    else {
        for (auto item : refTblNode(iNum).bNums)
            if (item != SENTINEL_BNUM)
                tempList.push_front(item);
    }
    
    return tempList;                                           // move, not copy
}

// Pre: node_file is open for update on fstream ns
// Called by: InodeTable()
void InodeTable::loadTbl()
{
    ns.read(reinterpret_cast<char *>(&avail), 
            NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK >> 3);

    for (uint32_t i = 0U; i != NUM_INODE_TBL_BLOCKS; ++i)
        for (uint32_t j = 0U; j != INODES_PER_BLOCK; ++j) {
            Inode tempNode;
            ns.read(reinterpret_cast<char *>(&tempNode.bNums), 
                    sizeof(tempNode.bNums));
            ns.read(reinterpret_cast<char *>(&tempNode.lkd),
                    sizeof(tempNode.lkd));
            ns.read(reinterpret_cast<char *>(&tempNode.crTime),
                    sizeof(tempNode.crTime));
            ns.read(reinterpret_cast<char *>(&tempNode.indirect),
                    sizeof(tempNode.indirect));
            ns.read(reinterpret_cast<char *>(&tempNode.iNum),
                    sizeof(tempNode.iNum));
            tbl[i][j] = tempNode;
        }
}

// Pre: file fileName (i.e., 'node_file') exists
//
// Stores inode table using FileShifter class as follows:
//
//   Creates 'fileName.tmp' containing new file table
//   Unlinks 'fileName'
//   Links 'fileName' to 'fileName.tmp'
//   Unlinks 'fileName.tmp'
//   
// Called by: ~InodeTable()
void InodeTable::storeTbl()
{
    auto bound_member_fn = bind(&InodeTable::doStoreTbl, ref(*this), _1);

    int32_t errVal = shifter.shiftFiles(fileName, bound_member_fn);

    assert(!errVal);
}

// pre: node_file is open for update on fstream ns
void InodeTable::doStoreTbl(fstream &ns)
{
    assert(ns);
    assert(ns.is_open());

    ns.write(reinterpret_cast<char *>(&avail),
             NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK >> 3);

    for (uint32_t i = 0U; i != NUM_INODE_TBL_BLOCKS; ++i)
        for (uint32_t j = 0U; j != INODES_PER_BLOCK; ++j) {
            Inode tempNode = tbl[i][j];
            ns.write(reinterpret_cast<char *>(&tempNode.bNums), 
                    sizeof(tempNode.bNums));
            ns.write(reinterpret_cast<char *>(&tempNode.lkd),
                    sizeof(tempNode.lkd));
            ns.write(reinterpret_cast<char *>(&tempNode.crTime),
                    sizeof(tempNode.crTime));
            ns.write(reinterpret_cast<char *>(&tempNode.indirect),
                    sizeof(tempNode.indirect));
            ns.write(reinterpret_cast<char *>(&tempNode.iNum),
                    sizeof(tempNode.iNum));
        }            

    clog << '\n' << tabs(1) << "Inode table stored.\n";
}
