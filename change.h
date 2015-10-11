#ifndef AAJ_CHANGE_STRUCT_FOR_MEMORY_PROGRAM_H
#define AAJ_CHANGE_STRUCT_FOR_MEMORY_PROGRAM_H

#include <array>
#include <deque>
#include <map>
#include <list>
#include <cstdint>
#include <iostream>
#include "aJTypes.h"

namespace MemModel {

    /************************************
        For any given Page, a std::deque<select_t> holds the numbers (offsets) 
        of the lines on that page which have been altered.
    *************************************/
    typedef std::array<lNum_t, sizeof(lNum_t) * 8U> select_t;

    /************************************
        A Change object is a container for altered lines. 

        Each line is of length 64 bytes. Each Change object 
        contains a deque holding the altered lines for one page, and a deque of 
        select_t that holds the corresponding line numbers.

        The last line of each page is occupied by that page's CRC value, and is 
        not presently used for any other purpose. So when a line number 0xFF is
        seen, we know there are no more lines in that Change.

        An empty Change object may contain a single select_t whose line numbers
        are all 0xFF.
    ************************************/
    struct Change {
        // Note: we want a move ctor for this class
        Change() = delete;
        // bool: if true, push an empty select_t onto deque selectors
        explicit Change(bNum_t bN, bool pushSelects = true);
        
        void addLine(bNum_t bN, lNum_t linNum, Line lin);
        void print(std::ostream &) const;
        inline bool linesAltered() const;

        bNum_t blkNum;
        // timeStamp is reset in MemMan::processRequest() const
        mutable uint64_t timeStamp = 0UL;
        uint32_t arrNext = 0UL;
        bool pushSelects;

        std::deque<select_t> selectors;         // select_t: an array<lNum_t, 8>
        std::deque<Line> newData;           // Line: an array<unsigned char, 64>
    };

    // Called by: MemMan::getReqType(), MemMan::wrtInSlot()
    bool Change::linesAltered() const
    {
        return !selectors.empty() && selectors.front()[0U] != 0xFF;
    }

    bool operator<(const Change &l, const Change &r);

    /************************************************
     A ChangeLog object is a container for Changes.
    ************************************************/
    struct ChangeLog {
        explicit ChangeLog(bool tSw = false): testSw(tSw) {}
        void addToLog(const Change &cg);
        void print(std::ostream &) const;
        uint32_t getCgLineCt() const { return cgLineCt; }
        uint32_t getLogSize() const { return theLog.size(); }
        bool isInLog(bNum_t bNum) const;
       
        // map key is block number
        std::map<bNum_t, std::list<Change>> theLog;
        bool testSw;                                      // test switch: c.l.a.
        uint32_t cgLineCt = 0UL;
        uint64_t lastCgWrtTime = 0UL;
    };

}

#endif
