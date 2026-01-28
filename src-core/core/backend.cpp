#define SATDUMP_DLL_EXPORT 1
#include "backend.h"

namespace backend
{
    SATDUMP_DLL float device_scale;

    SATDUMP_DLL float mouse_set_offset_x = 0;
    SATDUMP_DLL float mouse_set_offset_y = 0;

    SATDUMP_DLL std::function<void()> rebuildFonts;
    SATDUMP_DLL std::function<void(int, int)> setMousePos;
    SATDUMP_DLL std::function<std::pair<int, int>()> beginFrame;
    SATDUMP_DLL std::function<void()> endFrame;
    SATDUMP_DLL std::function<void(uint8_t *, int, int)> setIcon;

    SATDUMP_DLL std::function<std::string(std::string)> selectFolderDialog;
    SATDUMP_DLL std::function<std::string(std::vector<std::pair<std::string, std::string>>, std::string)> selectFileDialog;
    SATDUMP_DLL std::function<std::string(std::vector<std::pair<std::string, std::string>>, std::string, std::string)> saveFileDialog;
} // namespace backend
