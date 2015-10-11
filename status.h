#ifndef AAJ_MAINTAIN_STATUS_CLASS_H
#define AAJ_MAINTAIN_STATUS_CLASS_H

#include <string>
#include "fileShifter.h"

namespace Disk {

    /********************************
      Maintains a text file that tracks the last operation
      successfully completed by the system.
    *********************************/
    class Status {
    public:
        explicit Status(std::string sfn) : fileName(sfn) {}
        Status(const Status &) = delete;
        std::string rd() const;
        int32_t wrt(std::string s);

    private:
        int32_t replace(std::string s);
        void doReplace(std::string msg, std::fstream &fs);

        std::string fileName;
        FileSys::FileShifter shifter;
    };

}

#endif
