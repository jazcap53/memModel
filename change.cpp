#include <array>
#include <map>
#include <list>
#include <deque>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <cassert>
#include "change.h"
#include "aJTypes.h"

using namespace MemModel;

using std::array;
using std::map;
using std::list;
using std::deque;
using std::cerr;  using std::endl;  using std::ostream;
using std::flush;
using std::min;

////////////////////////////////////////
// MEMBER FUNCTIONS OF Change
////////////////////////////////////////

// Called by: Driver::makeRequests(), File::requestWrite(), 
//            Journal::purgeJrnl(), Journal::rdJrnl()
Change::Change(bNum_t bN, bool pushSelects): blkNum(bN)
{
    if (pushSelects) {
        select_t selTmp = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; 
        selectors.push_back(selTmp);
    } 
}

// Insert linNum into 'deque<select_t> selectors' and lin into 
//   'deque<Line> newData'.
// Pre: selectors.back()[arrNext] exists and equals 0xFF
// Called by: Driver::setUpCgs(), File::requestWrite()
void Change::addLine(bNum_t bN,
                     lNum_t linNum,               // value [0..LINES_PER_PAGE-1]
                     Line lin)           // array<unsigned char, BYTES_PER_LINE>
{
    assert(!selectors.empty() && !selectors.back().empty() &&
           selectors.back().size() >= arrNext+1 &&
           selectors.back()[arrNext] == 0xFF);
    assert(bN == blkNum);
    assert(linNum <= LINES_PER_PAGE - 1);
    
    selectors.back()[arrNext++] = linNum;
    if (arrNext == sizeof(select_t)) {
        select_t selTmp = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; 
        selectors.push_back(selTmp);
        arrNext = 0;
    }
    newData.push_back(lin);
}

// Output the altered lines for this Change.
// (Output dots for the non-printable characters.)
// Called by: ChangeLog::print()
void Change::print(ostream &os) const
{
    if (!newData.empty()) {
        deque<select_t>::const_iterator itSel = selectors.begin();
        uint32_t ixSel = 0UL;

        for (Line l : newData) {
            assert((*itSel)[ixSel] != 0xFF);

            os << "\t\t";
            for (unsigned char uc : l) {
                char c = static_cast<char>(uc);
                if (isprint(c))
                    os << c;
                else
                    os << '.';
            }
            os << "\n";
            ++ixSel;
            if (ixSel == sizeof(select_t)) {
                ixSel = 0;
                ++itSel;
            }
        }
        os << flush;
    }
}

////////////////////////////////////////
// NON-MEMBER FUNCTION
////////////////////////////////////////

namespace MemModel {

    bool operator<(const Change &l, const Change &r)
    {
        bool lLess = false;
        if (l.blkNum == r.blkNum)
            lLess = l.timeStamp < r.timeStamp;
        else
            lLess = l.blkNum < r.blkNum;
        return lLess;
    }
}

////////////////////////////////////////
// MEMBER FUNCTIONS OF ChangeLog
////////////////////////////////////////

// Called by: Journal::rdJrnl(), MemMan::wrtInSlot()
void ChangeLog::addToLog(const Change &cg)
{
    cgLineCt += cg.newData.size();

    if (!theLog.count(cg.blkNum))
        theLog[cg.blkNum] = list<Change>(1, cg);
    else
        theLog[cg.blkNum].push_back(cg);
}

// Call Change::print() for each Change in this ChangeLog
void ChangeLog::print(ostream &os) const
{ 
    for (auto cgLst : theLog)
        for (auto cg : cgLst.second) {
            if (!testSw)
                os << "\t\t(Block " << cg.blkNum << ")\n";
            cg.print(os);
            os << "\n";
        }

    os << flush;
}

bool ChangeLog::isInLog(bNum_t bNum) const
{
    return theLog.count(bNum);
}
