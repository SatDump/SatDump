#include "imgui/file_selection.h"

std::string getFilePath();
std::string getDirPath();
std::string getFilesavePath();

std::string selectInputFile(std::string name, std::string initialDir, std::vector<std::string> extensions)
{
    return getFilePath();
}

std::string selectDirectory(std::string name, std::string initialDir)
{
    return getDirPath();
}

std::string selectOutputFile(std::string name, std::string initialDir, std::vector<std::string> extensions)
{
    return getFilesavePath();
}

void bindFileDialogsFunctions()
{
    selectInputFileDialog = selectInputFile;
    selectDirectoryDialog = selectDirectory;
    selectOutputFileDialog = selectOutputFile;
}