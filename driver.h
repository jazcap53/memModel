#ifndef AAJ_MEMMODEL_DRIVER_H
#define AAJ_MEMMODEL_DRIVER_H

#include <fstream>
#include <random>
#include <string>
#include "myMemory.h"
#include "change.h"
#include "status.h"
#include "aJTypes.h"

using namespace std::chrono;
using namespace MemModel;


namespace Setup {

    /**********************************************************
      This class, along with the main() function, sets up the program to
      begin operation.
    **********************************************************/
    class Driver {
    public:
        Driver(int ac, char *av[]);
        Driver(const Driver &) = delete;
        ~Driver();
        void init(int ac, char *av[]);
        void makeRequests(MemMan &memMgr);
        void submitRequest(MemMan &memMgr, bNum_t bNum, bool doWrt);
        const std::string getDFileName() { return "disk_file"; }
        std::string getJFileName() { return "jrnl_file"; }
        std::string getFFileName() { return "free_file"; }
        std::string getNFileName() { return "node_file"; }
        std::string getSFileName() { return "status.txt"; }

        bool getVerbose() { return verbose; }
        bool getTest() { return test; }
        bool getLongRun() { return longRun; }
        uint64_t getTheSeed() { return theSeed; }

    private:
        void rdCLArgs(int ac, char *av[]);
        void wrtHeader(int ac, char *av[], const char *tag, 
                       std::ostream &os) const;
        void setUpCgs(Change &cg);
        void linCpy(Line &ln, const std::string &s) const;
        void displayHelp() const;

        // c. l. switches
        bool verbose = false, test = false, longRun = false, help = false;
 
        // Default seed for the Random Engine, when the -t switch is given.
        // This value, when 'memModel -l -t' is called after 'make fresh', 
        // produces output that includes an instance of cgLog long enough to
        // trigger a cgLog write and jrnl purge. (See MemMan::timedActs().)
        // The seed value may be altered from the command line. If the switch -t
        // is not given, the Random Engine is seeded with the system time.
        uint64_t theSeed = 7900UL;

        std::ofstream logfile;
        std::streambuf *backupBuf;   // back up clog's streambuf before redirect

        mutable Tabber tabs;
    };
   
} 

#endif
