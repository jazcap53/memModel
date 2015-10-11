#ifndef AAJ_MY_BOOST_CRC_H
#define AAJ_MY_BOOST_CRC_H

#include <cstdint>

namespace CRC {

    /****************************************
      This struct computes the CRC code for a page, and writes or reads
      it as the last 4 bytes of the page in big-endian fashion.
    ****************************************/
    struct BoostCRC {
        BoostCRC &operator=(const BoostCRC &) = delete;
        static const uint32_t polynom = 0x04c11db7U;
        static const uint32_t init_rem = 0xffffffff;

        static uint32_t getCode(unsigned char *data, uint32_t byteCt);

        template <class T>
        static void wrtBytesBigE(T num, unsigned char *p, size_t byt);
    };

    // write data in num to last byt bytes of array *p, in big-endian fashion
    template <class T>
    void BoostCRC::wrtBytesBigE(T num, unsigned char *p, size_t byt)
    {
        if (byt == sizeof(T)) {
            T mask = 0x000000FF;
            
            while (byt) {
                unsigned char uCVal = static_cast<unsigned char>(num & mask);
                *(p + byt - 1) = uCVal;
                num >>= 8;
                --byt;
            }
        }
    }

}

#endif
