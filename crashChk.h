#ifndef AAJ_PROGRAM_CRASH_CHECKER_H
#define AAJ_PROGRAM_CRASH_CHECKER_H

#include <fstream>
#include <string>

/***************************
 Reads contents of status_file after most recent program run.
***************************/
namespace Setup {

    class CrashChk {
    public:
        CrashChk();
        CrashChk(const CrashChk &) = delete;
        std::string getLastStatus() const { return lastStatus; }

    private:
        std::string lastStatus;
    };

}

#endif
