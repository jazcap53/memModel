#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include "aJUtils.h"

using namespace std::chrono;

using std::ostream;
using std::string;

uint64_t getCurTime(bool isInode)
{
    uint64_t ret = 0UL;
    if (isInode)
        // current time as milliseconds since epoch
        ret = duration_cast<milliseconds>(system_clock::now() - 
                                          time_point<system_clock>()
                                          ).count();
    else
        // current time as microseconds since program start
        ret = (duration_cast<microseconds>(steady_clock::now() - startup).
               count()); 
    return ret;
}

string Tabber::operator()(uint32_t num, bool newline)
{
    string s;

    if (num != Tabber::numTabs) { 
        if (newline)
            s += '\n';
        Tabber::numTabs = num;
    }

    for (int32_t i = 0; i != Tabber::numTabs; ++i)
        s += '\t';

    return s;
}
    
ostream &operator<<(ostream& os, const Tabber& tbr)
{
    os << tbr;
    return os;
}

uint32_t Tabber::numTabs = 0;
