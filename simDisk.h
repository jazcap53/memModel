#ifndef AAJ_SIMULATED_DISK_CLASS_H
#define AAJ_SIMULATED_DISK_CLASS_H

#include <array>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <list>
#include "journal.h"
#include "status.h"

namespace Disk {

    /********************
      Represents a disk sector.
    ********************/
    struct SimSector {
        unsigned char sect[u32Const::BLOCK_BYTES] = {0};
    };
        
    /********************
      Represents a hard disk as an array of sectors.
    ********************/
    class SimDisk {
        enum class fileType { disk, jrnl, free, node };
    public:
        SimDisk(Status *pstt,
                const std::string dfn, 
                const std::string jfn,
                const std::string ffn,
                const std::string nfn);
        SimDisk(const SimDisk &) = delete;
        ~SimDisk(); 

        std::fstream &getDs() { return ds; }
        const std::string &getDFileName() const;
        void doCreateBlock(SimSector &s, bNum_t rWSz);

    private:
        void init();
        bool readOrCreate(const std::string &fName, uint64_t fSz, bNum_t rWSz,
                          fileType fType);
        bool tryRead(const std::string &fName, uint64_t fSz, bNum_t rWSz,
                     fileType fType);
        bool tryCreate(const std::string &fName, bNum_t rWSz, fileType fType);

        void createDFile(std::ofstream &fs, bNum_t rWSz);
        void createJFile(std::ofstream &ofs, bNum_t rWSz);
        void createFFile(std::ofstream &ofs);
        void createNFile(std::ofstream &ofs);
        
        void errScan(std::ifstream &ifs, bNum_t rWSz);
        void processErrors();

        void createBlock(SimSector &s, bNum_t rWSz);

        std::array<SimSector, bNum_tConst::NUM_DISK_BLOCKS> theDisk;

        const std::string dFileName;                                // disk file
        const std::string jFileName;                             // journal file
        const std::string fFileName;                           // free list file
        const std::string nFileName;                                // node file
        std::fstream ds;                                      // ds: disk stream
        Status *p_stt;
        std::list<uint64_t> errBlocks;
    };

}

#endif
