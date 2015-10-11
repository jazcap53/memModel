#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <memory>
#include <array>
#include <string>
#include <cstring>
#include <bitset>
#include <cassert>
#include "myMemory.h"
#include "simDisk.h"
#include "journal.h"
#include "status.h"
#include "crc.h"
#include "inodeTable.h"

using std::ifstream;  using std::ofstream;  using std::fstream;  
using std::ios_base;  using std::cerr;  using std::endl;  using std::clog;
using std::unique_ptr;
using std::array;
using std::string;
using std::memset;
using std::bitset;

using namespace Disk;
using namespace CRC;

////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS of SimDisk
////////////////////////////////////////////////

// Post: (a disk file, a journal file, a free list file, and a node file exist
//       AND the disk file is open for binary update on stream ds)
//       OR the program has exited with a non-zero status.
// Called by: user (currently the main() function)
SimDisk::SimDisk(Status *pstt, const string dfn, const string jfn,
                 const string ffn, const string nfn) : 
    dFileName(dfn), jFileName(jfn), fFileName(ffn), nFileName(nfn), p_stt(pstt)
{
    init();
}

// Called by: SimDisk::SimDisk()
void SimDisk::init()
{
    p_stt->wrt("Initializing");

    bool successDisk = false, successJrnl = false, successFree = false,
        successNode = false, quit = false;

    successDisk = readOrCreate(dFileName, BLOCK_BYTES * NUM_DISK_BLOCKS, 
                               BLOCK_BYTES, fileType::disk);

    successJrnl = readOrCreate(jFileName, BLOCK_BYTES * PAGES_PER_JRNL,
                               BLOCK_BYTES, fileType::jrnl);

    successFree = readOrCreate(fFileName, BLOCK_BYTES * NUM_FREE_LIST_BLOCKS * 2
                               + sizeof(bNum_t), 0U, fileType::free);

    // node file size = 1 bit per inode + size of all inodes
    successNode = readOrCreate(nFileName,
                               (NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK >> 3) +
                               (BLOCK_BYTES * NUM_INODE_TBL_BLOCKS),
                               0U, fileType::node);
    
    if (successDisk && successJrnl && successFree && successNode) {
        ds.open(dFileName, ios_base::in | ios_base::out | ios_base::binary);
        if (!ds) {
            cerr << "ERROR: Error opening file " << dFileName 
                 << " for update (r/w) in " << __FUNCTION__ << endl;
            quit = true;
        }
    }
    else
        quit = true;

    if (quit)
        exit(1);
}

SimDisk::~SimDisk()
{
    if (ds.is_open()) 
        ds.close(); 
}

const std::string &SimDisk::getDFileName() const
{ 
    return dFileName; 
}

// Called by: Journal::emptyPurgeJrnlBuf()
void SimDisk::doCreateBlock(SimSector &s, bNum_t rWSz)
{
    createBlock(s, rWSz);
}

////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS of SimDisk
////////////////////////////////////////////////

// case 1: file named fName exists but is the wrong size
//       => exit
// case 2: file named fName exists and is the right size
//       => tryRead() is called
// case 3: no file named fName exists
//       => tryCreate() is called
bool SimDisk::readOrCreate(const string &fName, uint64_t fSz, bNum_t rWSz,
                  SimDisk::fileType fType)
{
    bool success = false;
    struct stat statBuf;
    int32_t statResult = stat(fName.c_str(), &statBuf);

    if (statResult == 0) {
        if (static_cast<uint64_t>(statBuf.st_size) != fSz) {
            cerr << "ERROR: Bad file size for " << fName << " in " 
                 << __FUNCTION__ << endl;
        }
        else {
            success = tryRead(fName, fSz, rWSz, fType);
        }
    }
    else {
        success = tryCreate(fName, rWSz, fType);
    }

    return success;
}


// For jrnl, free, and node files, just check the file exists and can be opened.
// For disk file, open and scan for errors.
// Called by: readOrCreate()
bool SimDisk::tryRead(const string &fName, uint64_t fSz, bNum_t rWSz,
                          fileType fType)
{
    assert(fType == fileType::disk || fType == fileType::jrnl ||
           fType == fileType::free || fType == fileType::node);

    bool success = true;
    ifstream ifs(fName, ios_base::in | ios_base::binary);

    if (ifs) {
        if (fType == fileType::disk) {
            errScan(ifs, rWSz);
			if (ifs.fail()) {
				cerr << "ERROR: Read error on disk file in " <<  __FILE__ 
					 << ", " << __FUNCTION__ << endl;
                success = false;
            }
            if (!errBlocks.empty())
                processErrors();
        }
    }
    else {
        cerr << "ERROR: Unable to open file " << fName << " for read in "
             << __FUNCTION__ << endl;
        success = false;
    }

    ifs.clear();

    if (ifs.is_open())
        ifs.close();

    if (ifs.fail())
        success = false;

    return success;
}

bool SimDisk::tryCreate(const string &fName, bNum_t rWSz, fileType fType)
{
    assert(fType == fileType::disk || fType == fileType::jrnl || 
           fType == fileType::free || fType == fileType::node);

    bool success = true;
    ofstream ofs(fName, ios_base::out | ios_base::binary);

    if (!ofs) {
        cerr << "ERROR: Unable to open " << fName << " for output in "
             << __FUNCTION__ << endl;
        success = false;
    }
    else {
        if (fType == fileType::disk) {
            createDFile(ofs, rWSz);
        }
        else if (fType == fileType::jrnl) {
            createJFile(ofs, rWSz);
        }
        else if (fType == fileType::free) {
            createFFile(ofs);
        }
        else if (fType == fileType::node) {
            createNFile(ofs);
        }

        ofs.close();
    }
        
    if (ofs.fail())
        success = false;

    return success;
}

// Create disk_file
// Called by: tryCreate()
void SimDisk::createDFile(ofstream &ofs, bNum_t rWSz)
{
    for (auto &s : theDisk) {
        createBlock(s, rWSz);
        ofs.write(reinterpret_cast<char *>(s.sect), rWSz);
    }
}

// Called by: createDFile(), doCreateBlock()
void SimDisk::createBlock(SimSector &s, bNum_t rWSz)
{
    // write crc code to end of block
    BoostCRC::wrtBytesBigE(0x738a0e78,
                           s.sect + BLOCK_BYTES - CRC_BYTES,
                           CRC_BYTES);
}

// Create jrnl_file
// Called by: tryCreate()
void SimDisk::createJFile(ofstream &ofs, bNum_t rWSz)
{
    array<unsigned char, BLOCK_BYTES> uCArr = {};
    
    for (uint32_t i = 0; i != PAGES_PER_JRNL; ++i)
        ofs.write(reinterpret_cast<char *>(&uCArr), rWSz);
}

// Create free file
// Called by: tryCreate()
void SimDisk::createFFile(ofstream &ofs)
{
    bNum_t bitsToSet = NUM_DISK_BLOCKS;
    uint32_t bytesToSet = bitsToSet >> 3;
    bool finished = false;

    array<bitset<BITS_PER_PAGE>, NUM_FREE_LIST_BLOCKS> bFrm;
    array<bitset<BITS_PER_PAGE>, NUM_FREE_LIST_BLOCKS> bTo;

    uint32_t arrIx = 0U;

    while (bytesToSet > BYTES_PER_PAGE) {
        bFrm[arrIx++].set();
        bytesToSet -= BYTES_PER_PAGE;
    }

    memset(&bFrm[arrIx], 0xFF, bytesToSet);
    
    bNum_t initPosn = 0U;

    ofs.seekp(0, ios_base::beg);
    ofs.write(reinterpret_cast<char *>(&bFrm), sizeof(bFrm));
    ofs.write(reinterpret_cast<char *>(&bTo), sizeof(bTo));
    ofs.write(reinterpret_cast<char *>(&initPosn), sizeof(bNum_t));

    uint64_t putPos = static_cast<uint64_t>(ofs.tellp());
    if (putPos == -1) {
        cerr << "system error in SimDisk::createFrFile()" << endl;
        exit(1);
    }
}

// Create node_file
// Called by: tryCreate()
void SimDisk::createNFile(ofstream &ofs)
{
    // fill array<bitset<>> InodeTable::avail
    const uint64_t avlArrSzBytes = NUM_INODE_TBL_BLOCKS * INODES_PER_BLOCK >> 3;
    array<unsigned char, avlArrSzBytes> avlArr;
    for (auto &c : avlArr)
        c = 0xFF;
    ofs.write(reinterpret_cast<char *>(&avlArr), avlArrSzBytes);
    
    // fill array<array<Inode>> InodeTable::tbl
    array<bNum_t, CT_INODE_BNUMS> bNumsFiller;
    array<bNum_t, CT_INODE_INDIRECTS> indirectFiller;
    bNumsFiller.fill(SENTINEL_BNUM);
    indirectFiller.fill(SENTINEL_BNUM);
    uint32_t lkdFiller = SENTINEL_32;
    uint64_t crTimeFiller = 0UL;

    for (uint32_t i = 0; i != NUM_INODE_TBL_BLOCKS; ++i)
        for (uint32_t j = 0; j != INODES_PER_BLOCK; ++j) {
            inNum_t ix = i * INODES_PER_BLOCK + j;
            ofs.write(reinterpret_cast<char *>(&bNumsFiller), 
                      sizeof(bNumsFiller));
            ofs.write(reinterpret_cast<char *>(&lkdFiller), 
                      sizeof(lkdFiller));
            ofs.write(reinterpret_cast<char *>(&crTimeFiller), 
                      sizeof(crTimeFiller));
            ofs.write(reinterpret_cast<char *>(&indirectFiller), 
                      sizeof(indirectFiller));
            ofs.write(reinterpret_cast<char *>(&ix), sizeof(inNum_t));
        }
}

// Check for corruption using CRC code
// Called by: tryRead()
void SimDisk::errScan(ifstream &ifs, bNum_t rWSz)
{
    for (auto it = theDisk.begin(); it != theDisk.end(); ++it) {
        ifs.read(reinterpret_cast<char *>(it->sect), rWSz);
        uint32_t code = BoostCRC::getCode(it->sect, rWSz);
        if (code)
            errBlocks.push_back(it - theDisk.begin());
    }
}

// Called by: tryRead()
void SimDisk::processErrors()
{
    for (uint64_t bN : errBlocks)
        clog << "WARNING: Found data error in block " << bN << " on startup.\n";
    clog << '\n';
}
