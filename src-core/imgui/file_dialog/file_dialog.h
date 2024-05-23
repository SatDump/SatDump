#pragma once

#include <string>
#include <functional>
#include <memory>

namespace satdump
{
    enum file_dialog_type_t
    {
        FILEDIALOG_OPEN,
    };

    class FileDialog
    {
    public:
        FileDialog(file_dialog_type_t type, std::string name, std::string directory) {}
        virtual bool ready() = 0;
        virtual std::string result() = 0;
    };

    struct RequestFileDialogEvent
    {
        std::vector<std::function<std::shared_ptr<FileDialog>(file_dialog_type_t, std::string, std::string)>> &constructors;
    };

    std::shared_ptr<FileDialog> create_file_dialog(file_dialog_type_t type, std::string name, std::string directory);
}