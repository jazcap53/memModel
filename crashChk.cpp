#include <fstream>
#include "crashChk.h"

using std::ifstream;

using namespace Setup;

// Called by: main()
CrashChk::CrashChk()
{
    ifstream ifs("status.txt");

    if (!ifs) {
        ifs.clear();
        ifs.open("status.tmp");
    }

    if (ifs) {
        getline(ifs, lastStatus);
        ifs.close();
    }  
}
