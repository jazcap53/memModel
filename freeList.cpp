#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <array>
#include <bitset>
#include "freeList.h"

using namespace FileSys;

using std::cerr;  using std::endl;  using std::cout;
using std::ios_base;
using std::string;
using std::array;
using std::bitset;

// Pre: ::fFileName exists and is of correct size
// Called by: FileMan()
FreeList::FreeList(string ffn): frs(ffn, ios_base::in | ios_base::out 
                                    | ios_base::binary)
{
    assert(frs);

    loadLst();
}

// Called by: ~FileMan()
FreeList::~FreeList()
{
    storeLst();

    if (frs.is_open())
        frs.close();
}

// Get free block numbers as bits from free file.
// Store them in bitsFrm and bitsTo.
// Called by: FreeList()
void FreeList::loadLst()
{
    frs.seekg(0, ios_base::beg);
    frs.read(reinterpret_cast<char *>(&bitsFrm), sizeof(bitsFrm));
    frs.read(reinterpret_cast<char *>(&bitsTo), sizeof(bitsTo));
    frs.read(reinterpret_cast<char *>(&fromPosn), sizeof(bNum_t));
}

// Called by: ~FreeList()
void FreeList::storeLst()
{
    frs.seekp(0, ios_base::beg);
    frs.write(reinterpret_cast<char *>(&bitsFrm), sizeof(bitsFrm));
    frs.write(reinterpret_cast<char *>(&bitsTo), sizeof(bitsTo));
    frs.write(reinterpret_cast<char *>(&fromPosn), sizeof(bNum_t));

    std::clog << '\n' << tabs(1) << "Free list stored.\n";
}

// Gets the next available block and returns its number or SENTINEL_BNUM
// Called by: FileMan::addBlock()
bNum_t FreeList::getBlk()
{
    if (fromPosn == NUM_DISK_BLOCKS) {           // if all blocks have been used
        if (bitsTo.any())                    // if any blocks have been returned
            refresh();            
    }

    bool found = false;

    if (fromPosn < NUM_DISK_BLOCKS) {
        bitsFrm.reset(fromPosn);
        found = true;
    }

    return (found ? fromPosn++ : SENTINEL_BNUM);
}

// Copy any available block numbers from bitsTo to bitsFrm
// Reset bitsTo
// Reset the block selector 'fromPosn'
// Post: if there is an available block:
//           fromPosn holds its number
//       else:
//           fromPosn == NUM_DISK_BLOCKS
// Called by: getBlk()
void FreeList::refresh()
{
    bitsFrm |= bitsTo;
    bitsTo.reset();

    fromPosn = 0U;

    for (bNum_t i = 0U; fromPosn < NUM_DISK_BLOCKS; ++i)
        if (!bitsFrm.test(i))
            ++fromPosn;
        else
            break;
}

// Put a block back on the free list when it is no longer in use.
// Called by: FileMan::remvBlock(), FileMan::deleteFile()
void FreeList::putBlk(bNum_t bN)
{
    assert(bN < NUM_DISK_BLOCKS);

    bitsTo.set(bN);
}
