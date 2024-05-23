#include "file_dialog.h"
#include "core/plugin.h"
#include "core/exception.h"

namespace satdump
{
    std::shared_ptr<FileDialog> create_file_dialog(file_dialog_type_t type, std::string name, std::string directory)
    {
        std::vector<std::function<std::shared_ptr<FileDialog>(file_dialog_type_t, std::string, std::string)>> constructors;
        eventBus->fire_event<RequestFileDialogEvent>({constructors});
        if (constructors.size() > 0)
            return constructors[0](type, name, directory);
        else
            throw satdump_exception("Could not create file dialog!");
    }
}