#ifndef AJ_THE_UTILS_FOR_MEMMODEL_H
#define AJ_THE_UTILS_FOR_MEMMODEL_H

#include <chrono>
#include <string>
#include <cstdint>

/******************************
  Utility functions and struct
******************************/

const std::chrono::steady_clock::time_point startup = 
    std::chrono::steady_clock::now();

// if isInode == false
//     returns microseconds from program start (steady_clock) 
// else
//     returns milliseconds from epoch start (system_clock)
uint64_t getCurTime(bool isInode = false);

// Tabber manipulator inserts num tabs into os. If newline == true, and
// num != numTabs, precedes tabs with a newline and resets numTabs to num
struct Tabber {
    static uint32_t numTabs;
    std::string operator()(uint32_t num, bool newline = false); 
};

std::ostream &operator<<(std::ostream& os, Tabber& tbr);

#endif
