#include <random>
#include <chrono>
#include <iostream>
#include <cstring>
#include <sstream>
#include <array>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include "myMemory.h"
#include "pageTable.h"
#include "memMan.h"
#include "simDisk.h"
#include "change.h"
#include "status.h"
#include "driver.h"

using std::knuth_b;  using std::uniform_int_distribution;
using std::cout;  using std::endl;  using std::cerr;  using std::ostream;
using std::flush;  using std::streambuf;  using std::ios_base;  using std::clog;
using std::strcpy;
using std::ostringstream;
using std::array;
using std::string;
using std::range_error;
using std::strtoul;
using std::isdigit;

using namespace std::chrono;
using namespace MemModel;
using namespace Setup;

/////////////////////////////////
// Public member functions
/////////////////////////////////

class MemMan;

Driver::Driver(int ac, char *av[])
{
    init(ac, av);
}

Driver::~Driver()
{
    clog << flush;
    clog.rdbuf(backupBuf);
    if (logfile.is_open())
        logfile.close();
}

void Driver::init(int ac, char *av[])
{
    rdCLArgs(ac, av);    
    if (help) {
        displayHelp();
        exit(0);
    }

    logfile.open("output.txt", ios_base::out);
    streambuf *fileStrmBuf = logfile.rdbuf();
    backupBuf = clog.rdbuf();
    clog.rdbuf(fileStrmBuf);

    wrtHeader(ac, av, "OUTPUT", clog);
    wrtHeader(ac, av, "ERROR OUTPUT", cerr);
}

/////////////////////////////////
// Private member functions
/////////////////////////////////

// Called by: main()
void Driver::rdCLArgs(int ac, char *av[])
{
   while (--ac && !help) {
        char cla = '\0';
        if ((*++av)[0] == '-') {
            cla = (*av)[1];
            switch (cla) {
            case 'v':
                verbose = true;
                break;
            case 't':     // use default seed, print block number of each change
                test = true;
                break;
            case 's':                             // use subsequent arg for seed
                if (!isdigit((*++av)[0])) {                       // input error
                    help = true;
                    --ac;
                    break;
                }
                theSeed = strtoul(*av, nullptr, 0);
                --ac;
                break;
            case 'L':
            case 'l':  
                longRun = true;
                break;
            case 'h':
                help = true;
                break;
            default:
                cerr << "ERROR: Bad command line argument" << '\n';
            }
        }
    }
}

// Write header line to output
// Called by: main()
void Driver::wrtHeader(int ac, char *av[], const char *tag, ostream &os) const
{
    os << tabs(0) << tag << ": ";

    for (int i = 0; i != ac; ++i) {
        os << *av++ << ' ';
    }

    time_t tt = system_clock::to_time_t(system_clock::now());
    os << ": " << ctime(&tt);
}

void  Driver::displayHelp() const
{
    cout << "\n\tmemModel options:\n\n";

    cout << "\t-h   Help       Print this help and exit.\n\n"
         << "\t-l\n"
         << "\t-L   Long run   Run the program long enough to make the\n"
         << "\t                journal file wrap around.\n\n"
         << "\t-s   Seed       Seed the random number generator with the\n"
         << "\t                non-negative integer that appears as the next\n"
         << "\t                command line argument.\n\n"
         << "\t-t   Test       Use the default seed (or the argument to -s,\n"
         << "\t                if given) for the random number generator.\n"
         << "\t                Make the first line of each output change hold\n"
         << "\t                its block number.\n\n"
         << "\t-v   Verbose    Send some extra debugging information to stdout."
         << "\n" << endl;
}
