#ifndef AAJ_SHIFT_THE_FILES_H
#define AAJ_SHIFT_THE_FILES_H

#include <string>
#include <fstream>
#include <functional>

namespace FileSys {

    /*****************************
      This mix-in allows a file to be updated by creating a temporary copy,
      updating the copy, and replacing the original with the temporary.
    *****************************/
    struct FileShifter {
        FileShifter &operator=(const FileShifter &) = delete;
        int32_t shiftFiles(std::string fName, 
                       std::function<void(std::fstream &)> action);
    };

}

#endif
