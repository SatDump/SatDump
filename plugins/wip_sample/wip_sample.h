#pragma once

#include "common/widgets/menuitem_fileopen.h"
#include "handlers/handler.h"
#include "imgui/imgui.h"

namespace satdump
{
    class WipSampleHandler : public handlers::Handler
    {
    protected:
        void drawMenu();
        void drawContents(ImVec2 win_size);
        void drawMenuBar();

    private:
        // Here you have anything that you want to use inside the plugin, but don't want to make it available to other plugins/code inside satdump
        widget::MenuItemFileOpen file_open_menu;

    public:
        // Here you'll put stuff that is accessible by other plugins/code inside satdump
        WipSampleHandler();
        ~WipSampleHandler();

    public:
        std::string getID() { return "wip_sample_handler"; }
        std::string getName() { return "WIP Sample"; }
    };
}; // namespace satdump
