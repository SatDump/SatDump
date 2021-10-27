#define SATDUMP_DLL_EXPORT 1
#include "file_selection.h"

SATDUMP_DLL std::function<std::string(std::string, std::string, std::vector<std::string>)> selectInputFileDialog;
SATDUMP_DLL std::function<std::string(std::string, std::string)> selectDirectoryDialog;
SATDUMP_DLL std::function<std::string(std::string, std::string, std::vector<std::string>)> selectOutputFileDialog;
