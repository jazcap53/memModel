#ifndef AAJ_FILE_MANAGER_STUFF_ETC_H
#define AAJ_FILE_MANAGER_STUFF_ETC_H

#include <string>
#include <limits>
#include "freeList.h"
#include "inodeTable.h"
#include "change.h"
#include "aJTypes.h"

namespace MemModel {
    class MemMan;
}

namespace FileSys {

    /***************************
      FileMan creates a file by assigning an inode num at Client request.
      Client may request addBlock or remvBlock to/from a given File, or
      request file read or write.
    ***************************/
    class FileMan {
    public:
        FileMan(std::string nfn, std::string ffn, MemModel::MemMan *pmm) :
            iTbl(nfn), frLst(ffn), p_mM(pmm) {}
        FileMan(const FileMan &) = delete;
        FileMan &operator=(const FileMan &) = delete;
        FileMan(FileMan &&) = delete;
        FileMan &operator=(FileMan &&) = delete;
        inNum_t createFile();
        bool deleteFile(uint32_t cliId, inNum_t iNum);
        // void openFile();                                             // N.Y.I
        // void closeFile();                                            // N.Y.I
        // void saveFile(inNum_t iNum);                                 // N.Y.I
        inNum_t countFiles() const;
        bNum_t countBlocks(inNum_t iNum) const;
        bool fileExists(inNum_t iNum) const;
        bool blockExists(inNum_t iNum, bNum_t bNum) const;         // Not in use
        bNum_t addBlock(uint32_t cliId, inNum_t iNum);
        bool remvBlock(uint32_t cliId, inNum_t iNum, bNum_t tgt);
        void remvBlockClean(bNum_t tgt);
        const Inode &getInode(inNum_t iNum) const;
        void submitRequest(bool doWrt, uint32_t cliId, inNum_t iNum,
                           const MemModel::Change &cg);
        void doStoreInodes();
        void doStoreFreeList();

    private:
        InodeTable iTbl;
        FreeList frLst;
        bool anyDirty = false;
        MemModel::MemMan *p_mM;
        Tabber tabs;
    };

}

#endif
