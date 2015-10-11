#include <boost/crc.hpp>                             // for boost::augmented_crc
#include <cstdint>
#include "crc.h"

using namespace CRC;

uint32_t BoostCRC::getCode(unsigned char *data, uint32_t byteCt)
{
    return boost::augmented_crc<sizeof(uint64_t) * 8U, polynom>(data, byteCt, 
                                                                init_rem);
}
