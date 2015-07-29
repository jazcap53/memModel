#include <string>
#include <algorithm>
#include <fstream>
#include <ios>
#include <iostream>
#include <functional>
#include <cassert>
#include "status.h"
#include "fileShifter.h"

using std::string;
using std::ifstream;  using std::ofstream;  using std::fstream;
using std::ios;
using std::cerr;  using std::endl;
using std::ref;

using namespace Disk;
using namespace std::placeholders;

string Status::rd() const
{
    string condition;

    ifstream ifs(fileName, ios::in);
    if (!ifs)
        condition = "ERROR: Can not open status file for read in Status::rd().";
    else {
        getline(ifs, condition);
        ifs.close();
    }

    return condition;
}

int32_t Status::wrt(string msg)
{
#ifdef AWAIT_CTRL_C
    ofstream ofs2("statusDebug.txt", ios::out | ios::app);
    if (!ofs2) {
        cerr << "failed to open statusDebug.txt for write" << endl;
        exit(1);
    }
    ofs2 << msg << endl;
#endif

    int32_t hadError = 0;
    ifstream ifs(fileName, ios::in);

    if (ifs.fail()) {
        ofstream ofs(fileName, ios::out);
        if (!ofs) {
            cerr << "ERROR: Can't open file " << fileName 
                 << " for write of message " << msg << " in " << __FUNCTION__ 
                 << ", " << __LINE__ << endl;
        }
        else {
            ofs << msg << endl;
            ofs.close();
        }
    }
    else {
        ifs.close();
        hadError = replace(msg);
    }

#ifdef AWAIT_CTRL_C
    ofs2.close();
#endif
    return hadError;
}

// Pre: file 'fileName' i.e. 'status.txt' exists
//
// Stores status file using FileShifter class as follows:
//
//   Creates 'fileName.tmp' containing new status
//   Unlinks 'fileName'
//   Links 'fileName' to 'fileName.tmp'
//   Unlinks 'fileName.tmp'
//   
// Returns: 0 for no error
//          negative value for error
// Called by: wrt()
int32_t Status::replace(string s)
{
    auto bound_member_fn = bind(&Status::doReplace, ref(*this), s, _1);

    int32_t errVal = shifter.shiftFiles(fileName, bound_member_fn);

    return errVal;
}

void Status::doReplace(string msg, fstream &fs)
{
    assert(fs);
    assert(fs.is_open());

    fs << msg << endl;
}
