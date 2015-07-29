#include "wipeList.h"
#include "aJTypes.h"

using namespace Disk;

WipeList::WipeList()
{
    clearArray();
}

// Called by: Journal::setWiperDirty()
void WipeList::setDirty(bNum_t bNum)
{
    dirty.set(bNum);
}

// Called by:Journal::emptyPurgeJrnlBuf(), Journal::doWipeRoutine()
bool WipeList::isDirty(bNum_t bNum)
{
    return dirty.test(bNum);
}

// Called by: Journal::doWipeRoutine()
void WipeList::clearArray()
{
    dirty.reset();
}

// Called by: Journal::doWipeRoutine()
bool WipeList::isRipe()
{
    uint32_t numToWipe = 0U;

    numToWipe = dirty.count();

    return numToWipe >= DIRTY_BEFORE_WIPE;
}
