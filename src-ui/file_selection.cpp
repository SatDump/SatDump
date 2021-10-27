#include "imgui/file_selection.h"
#include "portable-file-dialogs.h"

std::string selectInputFile(std::string name, std::string initialDir, std::vector<std::string> extensions)
{
    auto result = pfd::open_file(name, initialDir, extensions, pfd::opt::none);
    while (result.ready(1000))
    {
    }

    if (result.result().size() > 0)
        return result.result()[0];
    else
        return std::string();
}

std::string selectDirectory(std::string name, std::string initialDir)
{
    auto result = pfd::select_folder(name, initialDir);
    while (result.ready(1000))
    {
    }

    if (result.result().size() > 0)
        return result.result();
    else
        return std::string();
}

std::string selectOutputFile(std::string name, std::string initialDir, std::vector<std::string> extensions)
{
    auto result = pfd::save_file(name, initialDir, extensions, pfd::opt::none);
    while (result.ready(1000))
    {
    }

    if (result.result().size() > 0)
        return result.result();
    else
        return std::string();
}

void bindFileDialogsFunctions()
{
    selectInputFileDialog = selectInputFile;
    selectDirectoryDialog = selectDirectory;
    selectOutputFileDialog = selectOutputFile;
}