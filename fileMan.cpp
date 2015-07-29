#include <iostream>
#include <forward_list>
#include "fileMan.h"
#include "memMan.h"

using namespace FileSys;

using std::clog;  using std::endl;
using std::shared_ptr;
using std::forward_list;

// Assigns an inode for the new file, but does not assign any disk blocks.
// Called by: Client::reqCreateFile()
inNum_t FileMan::createFile()
{
    inNum_t ret = iTbl.assignInN();    
    if (ret == SENTINEL_INUM)
        clog << "Unable to create file in FileMan::createFile(); "
             << "file number limit reached.\n";
    else
        clog << tabs(2, true) << "File created with inode #" << ret << '\n';

    return ret;
}

// For the given inode:
//   Clear the inode
//   Return its blocks to free list
//   Release the inode
// Called by: Client::reqDeleteFile()
bool FileMan::deleteFile(uint32_t cliId, inNum_t iNum)
{
    bool success = false;

    bool lkd = iTbl.nodeLocked(iNum);
    if (lkd) {
        clog << "Unable to delete file " << iNum << " for client " 
             << cliId << " at time " << getCurTime() << ": file locked.\n";
    }
    else if (!fileExists(iNum)) {
        clog << "Unable to delete file " << iNum << " for client " 
             << cliId << " at time " << getCurTime() << ": no such file.\n";
    }
    else {
        forward_list<bNum_t> bNList = iTbl.listAllBlkN(iNum);
        for (auto item : bNList) {
            frLst.putBlk(item);
            p_mM->evictThisPage(item);
        }

        iTbl.releaseAllBlkN(iNum);
        iTbl.releaseInN(iNum);

        clog << tabs(2, true) << "File deleted with inode #" << iNum 
             << " for client " << cliId << " at time " << getCurTime() << '\n';

        success = true;
    }

    return success;
}

inNum_t FileMan::countFiles() const
{
    inNum_t files = 0U;                                // number of files in use

    for (uint32_t i = 0U; i != NUM_INODE_TBL_BLOCKS; ++i)
        files += iTbl.avail[i].size() - iTbl.avail[i].count();
    
    return files;
}

bNum_t FileMan::countBlocks(inNum_t iNum) const
{
    bNum_t ct = 0U;

    for (auto item : iTbl.refTblNode(iNum).bNums)
        if (item != SENTINEL_BNUM)
            ++ct;

    return ct;
}

bool FileMan::fileExists(inNum_t iNum) const
{
    if (iNum == SENTINEL_INUM)
        return false;

    return iTbl.nodeInUse(iNum);
}

// Not in use
bool FileMan::blockExists(inNum_t iNum, bNum_t bNum) const
{
    bool exists = false;

    if (iNum == SENTINEL_INUM || bNum == SENTINEL_BNUM)
        ;
    else if (!fileExists(iNum))
        ;
    else {
        const Inode tmp = iTbl.refTblNode(iNum);
        for (uint32_t i = 0; i != CT_INODE_BNUMS; ++i)
            if (tmp.bNums[i] == bNum) {
                exists = true;
                break;
            }
    }

    return exists;
}

// If inode iNum is not locked by another client, and there are free blocks, and
// there is room in the inode for another block:
//   take the first free block off of the free list
//   return its number
// Else:
//   return SENTINEL_BNUM 
// Called by: Client::reqAddBlock()
bNum_t FileMan::addBlock(uint32_t cliId, inNum_t iNum)
{
    bNum_t bNum = SENTINEL_BNUM;

    bool lkd = iTbl.nodeLocked(iNum);
    if (lkd) {
        clog << "Unable to add block to file " << iNum << " for client " 
             << cliId << " at time " << getCurTime() << ": file locked.\n";
    }
    else {
        bNum = frLst.getBlk();  // if successful, takes block bNum off free list

        if (bNum == SENTINEL_BNUM)
            clog << "Unable to add block to inode " << iNum 
                 << " for client " << cliId << " at time " << getCurTime() 
                 << ": no free blocks\n";
        else {
            if (iTbl.assignBlkN(iNum, bNum)) {
                p_mM->p_j->doWipeRoutine(bNum, this);

                clog << tabs(2, true) << "Block " << bNum << " added to inode " 
                     << iNum << " for client " << cliId << " at time " 
                     << getCurTime() << '\n';
            }
            else {
                bNum = SENTINEL_BNUM;
                clog << "Unable to add block to inode " << iNum 
                     << " for client " << cliId << " at time " << getCurTime() 
                     << ": no space in inode\n";
            }
        }
    }

    return bNum;
}

// Remove block tgt from inode number iNum
// Called by: Client::reqRemvBlock()
bool FileMan::remvBlock(uint32_t cliId, inNum_t iNum, bNum_t tgt)
{
    bool ret = true;
    enum { inodeLocked, inodeNotInUse, blkNotFound } reason; 

    bool lkd = iTbl.nodeLocked(iNum);
    if (lkd)
        ret = false, reason = inodeLocked;
    else {
        if (iTbl.nodeInUse(iNum)) {
            if (iTbl.releaseBlkN(iNum, tgt))         // returns true for success
                remvBlockClean(tgt);
            else
                ret = false, reason = blkNotFound;
        }
        else
            ret = false, reason = inodeNotInUse;

        if (ret)
            clog << tabs(2) << "Block " << tgt << " removed from inode " 
                 << iNum << " for client " << cliId << " at time " 
                 << getCurTime() << '\n';
        else {
            clog << "Unable to remove block " << tgt << " from inode " << iNum
                 << " for client " << cliId << " at time " << getCurTime() 
                 << ":\n";
            switch (reason) {
            case inodeNotInUse:
                clog << "\tInode not in use.\n";              break;
            case blkNotFound:
                clog << "\tBlock not found in inode\n";       break;
            case inodeLocked:
                clog << "\tFile locked.\n";                   break;
            }
        }
    }
    return ret;
}

// Called by: remvBlock()
void FileMan::remvBlockClean(bNum_t tgt)
{
    frLst.putBlk(tgt);
    p_mM->evictThisPage(tgt);
    // if tgt is in ChangeLog::theLog or in Journal::blksInJrnl
    if (p_mM->p_cL->isInLog(tgt) || p_mM->p_j->isInJrnl(tgt))
        p_mM->p_j->setWiperDirty(tgt); // mark released blk as dirty
}

// Pre: iNum != SENTINEL_INUM
// Called by: Client::makeRequests(), Client::rndBlkNum()
const Inode &FileMan::getInode(inNum_t iNum) const
{
    assert(iNum != SENTINEL_INUM);
 
    return iTbl.refTblNode(iNum); 
}

// If no one holds the lock on this inode OR cliId holds the lock on this inode,
// submit the request to the memory manager for processing.
// Called by: Client::makeRequests()
void FileMan::submitRequest(bool doWrt, uint32_t cliId, inNum_t iNum,
                            const MemModel::Change &cg)
{
    bool locked = iTbl.nodeLocked(iNum);

    if (!locked) {
        p_mM->processRequest(cg, this);
        anyDirty = true;
    }
}

void FileMan::doStoreInodes()
{
    iTbl.storeTbl();
}

void FileMan::doStoreFreeList()
{
    frLst.storeLst();
}
