#include <string>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <functional>
#include "fileShifter.h"
#include "aJUtils.h"

using std::string;
using std::fstream;  using std::ofstream;
using std::cerr;
using std::ios_base;
using std::function;

using namespace FileSys;

// Pre: file 'fName' exists
//
// Creates 'fName.tmp' with new file contents
// Unlinks 'fName'
// Links 'fName' to 'fName.tmp'
// Unlinks 'fName.tmp'
//
// Returns: 0 for no error
//          negative value for error
int32_t FileShifter::shiftFiles(string fName, function<void(fstream &)> action)
{
    int32_t hadError = 0;
    string tmpFile = fName + ".tmp";

    fstream fstrmTemp(tmpFile, ios_base::out | ios_base::trunc);
    if (fstrmTemp) {
        action(fstrmTemp);
        fstrmTemp.close();
        hadError = 2 * unlink(fName.c_str());
        if (!hadError)
            hadError = 3 * link(tmpFile.c_str(), fName.c_str());
        if (!hadError)
            hadError = 4 * unlink(tmpFile.c_str());
    }
    else
        hadError = -1;

    if (hadError) {
        cerr << "ERROR: Error value of " << hadError << " in " << __FILE__ 
             << ", " << __LINE__ << " at time " << getCurTime() << '\n';
    }

    return hadError;
}
