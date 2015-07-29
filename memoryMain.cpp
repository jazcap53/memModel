#include "crashChk.h"
#include "myMemory.h"
#include "memMan.h"
#include "simDisk.h"
#include "change.h"
#include "status.h"
#include "driver.h"
#include "fileMan.h"
#include "client.h"

using namespace MemModel;
using namespace Disk;
using namespace Setup;

int main(int argc, char *argv[])
{
    CrashChk crChk;               // help system recover gracefully from a crash
    Driver drvr(argc, argv);                               // set up the program
    Memory mainMem;          // set up data structures for pages and main memory
    ChangeLog myCgLog(drvr.getTest());     // track changes for write to journal
    Status myStts(drvr.getSFileName());    // maintain a status file for program
    // files for disk, journal, free list, and inode table
    SimDisk myDsk(&myStts, drvr.getDFileName(), drvr.getJFileName(),
                  drvr.getFFileName(), drvr.getNFileName());  
    // write changes from change log to journal, and from journal to disk
    Journal myJrnl(drvr.getJFileName(), &myDsk, &myCgLog, &myStts, &crChk);
    MemMan memMgr(&mainMem, &myDsk, &myJrnl, &myCgLog, &myStts, 
                  drvr.getVerbose());            // coordinate memory management
    FileSys::FileMan fileMgr(drvr.getNFileName(), drvr.getFFileName(), &memMgr);
    User::Client client01(0U, &fileMgr, &drvr);

    client01.makeRequests();
}
