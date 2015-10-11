#ifndef AJ_ARR_BIT_CLASS_H
#define AJ_ARR_BIT_CLASS_H

#include <array>
#include <bitset>
#include "aJTypes.h"

namespace FileSys {
    class FileMan;
}

/*******************************************************************
  Wrapper for an array of bitsets.
  Presents a bitset-like interface to the combined data structure.
  Note: some functions are not yet called. They are present for
        completeness.
*******************************************************************/

template <typename T, // array size type 
          typename U, // bitset size type
          typename V, // index type
          T aSz,      // array size
          U bSz>      // bitset size
class ArrBit {
    friend class FileSys::FileMan;
public:
    bool test(V ix) const;
    void set();
    void set(V ix);
    void reset();
    void reset(V ix);
    U size() const;
    V count() const;
    bool all() const;
    bool any() const;
    void flip();
    void flip(V ix);
    bool none() const;
    ArrBit<T,U,V,aSz,bSz> operator|=(const ArrBit<T,U,V,aSz,bSz> &rhs);

private:
    std::array<std::bitset<bSz>, aSz> arBt;

    // Note: nonstandard implementation of these two functions, which is why
    //       the have been declared private. They are for the use of 
    //       operator|=() *only*.
    const std::bitset<bSz> &operator[](uint32_t i) const;
    std::bitset<bSz> &operator[](uint32_t i);
    
};


#include "arrBit.cpp"

#endif
