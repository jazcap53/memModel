#ifndef AJ_MY_TYPES_FOR_MEMMODEL_H
#define AJ_MY_TYPES_FOR_MEMMODEL_H

#include <cstdint>
#include <array>
#include <limits>

/*************************************************
  typedefs and enums used throughout the program
*************************************************/

using bNum_t = uint32_t;        // block number type
using lNum_t = unsigned char;   // line number type
using inNum_t = uint32_t;       // inode number type

using Line = std::array<unsigned char, 64>;

enum {SENTINEL_32 = std::numeric_limits<uint32_t>::max()};
enum {SENTINEL_INUM = std::numeric_limits<inNum_t>::max()};
enum {SENTINEL_BNUM = std::numeric_limits<bNum_t>::max()};

enum u32Const : uint32_t {
    BLOCK_BYTES = 4096U,
    BYTES_PER_LINE = 64U, 
    BYTES_PER_PAGE = BLOCK_BYTES,
    BITS_PER_PAGE = BYTES_PER_PAGE << 3,
    CG_LOG_FULL = BYTES_PER_PAGE << 1, 
    CRC_BYTES = 4U,
    CT_INODE_BNUMS = 9U,
    CT_INODE_INDIRECTS = 3U, 
    PAGES_PER_JRNL = 16U,
    JRNL_SIZE = PAGES_PER_JRNL * BYTES_PER_PAGE,
    NUM_MEM_SLOTS = 32U,
    MAX_BLOCKS_PER_FILE = (NUM_MEM_SLOTS << 1) - (NUM_MEM_SLOTS >> 1),
    NUM_FREE_LIST_BLOCKS = 1U, 
    NUM_INODE_TBL_BLOCKS = 2U,
    NUM_WIPE_PAGES = 1U
};    

enum lNum_tConst : lNum_t {
    INODES_PER_BLOCK = 64U,
    LINES_PER_PAGE = 63U                             // 64th line is empty + crc
};
        
enum bNum_tConst : bNum_t {
    NUM_DISK_BLOCKS = 256U                          // should be a multiple of 8
};

#endif
