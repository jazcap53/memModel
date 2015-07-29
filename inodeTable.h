#ifndef AAJ_THE_INODE_TABLE_H
#define AAJ_THE_INODE_TABLE_H

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <forward_list>
#include "fileShifter.h"
#include "aJTypes.h"
#include "aJUtils.h"
#include "arrBit.h"

namespace FileSys {

    /*****************************
      64-byte Inode fits in a Line; fields are ordered to prevent data bloat
      Each Inode holds its id number, creation time, the numbers of up to 9
      blocks associated with the inode id number, the numbers of up to 3 UNIX-
      style indirect blocks (N.Y.I.), and the id number of the client, if any,
      who holds a write lock on the inode.
    *****************************/
    struct Inode {
        std::array<bNum_t, u32Const::CT_INODE_BNUMS> bNums;       // 9 * 4 bytes
        uint32_t lkd;                                        // 'lkd': locked by
        uint64_t crTime;                         // 'crTime': node creation time
        std::array<bNum_t, u32Const::CT_INODE_INDIRECTS> indirect;    // 3 * 4 B
        inNum_t iNum;                                                 // 4 bytes
    };
    
    /*****************************
      Each available inode is represented by a set bit in array<bitset<>> 
      'avail'. When an inode is requested, if any are available, the lowest 
      available inode number is returned and its bit is reset. If no inode 
      number is available, SENTINEL_INUM is returned.
      
      The blocks themselves are stored in array<Inode> 'tbl'.
      Arrays 'avail' and 'tbl' are saved to file at regular intervals.
    *****************************/
    class InodeTable {
        friend class FileMan;

    private:
        explicit InodeTable(std::string nfn);
        ~InodeTable();

        Inode &refTblNode(inNum_t iNum);
        const Inode &refTblNode(inNum_t iNum) const;
        inNum_t assignInN();
        void releaseInN(inNum_t nNum);
        bool nodeInUse(inNum_t iNum) const;
        bool nodeLocked(inNum_t iNum) const;
        bool assignBlkN(inNum_t iNum, bNum_t blk);
        bool releaseBlkN(inNum_t iNum, bNum_t tgt);
        void releaseAllBlkN(inNum_t iNum);
        std::forward_list<bNum_t> listAllBlkN(inNum_t iNum);
        void loadTbl();
        void storeTbl();
        void doStoreTbl(std::fstream &ns);

        std::fstream ns;
        std::string fileName;
        FileShifter shifter;
        Tabber tabs;

        // a bit for each inode
        // u32Const: array size type; lNum_tConst: bitset size type
        ArrBit<u32Const, lNum_tConst, inNum_t, NUM_INODE_TBL_BLOCKS, 
               INODES_PER_BLOCK> avail;
        
        // the inodes themselves
        using InodeBlock = std::array<Inode, lNum_tConst::INODES_PER_BLOCK>;
        std::array<InodeBlock, u32Const::NUM_INODE_TBL_BLOCKS> tbl;
    };

}

#endif
