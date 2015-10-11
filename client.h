#ifndef AAJ_CLIENT_THE_USER_H
#define AAJ_CLIENT_THE_USER_H

#include <random>
#include <limits>
#include <string>
#include <vector>
#include "fileMan.h"
#include "driver.h"
#include "aJTypes.h"

using namespace FileSys;
using namespace Setup;

class FileMan;

namespace User {

    using FileSys::FileMan;

    /***************************
       Client is a place holder for a N.Y.I. group of classes that
       will make requests of the program through the FileMan class.
       It implements requests to create and delete files, add or
       delete blocks to/from a file, and read or write a file.
    ***************************/
    class Client {
        typedef std::uniform_int_distribution<bNum_t>  dist_bNum;
        typedef std::uniform_int_distribution<uint64_t> dist_u64; 
        typedef std::uniform_int_distribution<int32_t>  dist_i32;
        typedef std::uniform_int_distribution<lNum_t>  dist_lNum;
        typedef std::uniform_int_distribution<uint32_t>  dist_u32;
        typedef std::uniform_int_distribution<inNum_t>  dist_inNum;

        // used by init() to specify run length
        // convenient values chosen by trial-and-error
        const uint32_t SHORT_RUN = 256;
        const uint32_t RUN_FACTOR = 112;            

    public:
        Client(uint32_t id, FileMan *pfm, Driver *pdrvr);
        void init();
        void makeRequests();
        uint32_t getMyId() { return myId; }

    private:
        void createOrDelete();
        void deleteOrCreate();
        void addRndBlock(int32_t *p_i);
        void remvRndBlock(int32_t *p_i);
        void makeRWRequest(int32_t *p_i);
        inNum_t reqCreateFile();
        bool reqDeleteFile(inNum_t iNum);
        inNum_t reqCountFiles() const;
        bNum_t reqCountBlocks(inNum_t iNum) const;
        bool reqFileExists(inNum_t iNum) const;
        bNum_t reqAddBlock(inNum_t iNum);
        bool reqRemvBlock(inNum_t iNum, bNum_t bNum);
        const Inode &reqGetInode(inNum_t iNum) const;
        void reqSubmitRequest(bool doWrt, uint32_t cliId, inNum_t iNum,
                           const MemModel::Change &cg);

        inNum_t rndFileNum() const;
        bNum_t rndBlkNum(inNum_t tgtNdNum) const;
        void setUpCgs(Change &cg);
        void linCpy(Line &ln, const std::string &s) const;
        void rndDelay();
        bool remvFrmMyOpenFiles(inNum_t iNum);            // not in use

        uint32_t myId;
        int32_t numRequests = 0;                    // will be changed by init()
        // 1st element of each inner vector is the inode num
        // remaining elements of each inner vector are block nums
        std::vector<std::vector<uint32_t>> myOpenFiles;            // not in use

        FileMan *p_fM;
        Driver *p_drvr;

        // if a random int32_t in [loChooser, hiChooser] is >= rdPct: write
        // else: read
        const int32_t rdPct = 60;  // convenient value chosen by trial and error
        const int32_t loChooser = 0, hiChooser = 99;
        const bNum_t loPage = 0, hiPage = bNum_tConst::NUM_DISK_BLOCKS - 1;
        const uint64_t minDelay = 0, maxDelay = 850;
        const int32_t actions = 99;
        const inNum_t hiInode = (u32Const::NUM_INODE_TBL_BLOCKS *
                                 lNum_tConst::INODES_PER_BLOCK - 1);
        std::knuth_b kRE;

        dist_bNum uID_whichBlock;
        dist_i32 uID_rdOrWrt;
        dist_u64 uID_delay;
        dist_i32 uID_actions;
        dist_inNum uID_whichInode;
    };

}

#endif
