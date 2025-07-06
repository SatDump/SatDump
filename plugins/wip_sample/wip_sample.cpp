#include "wip_sample.h"

namespace satdump
{
    WipSampleHandler::WipSampleHandler()
    {
        handler_tree_icon = u8"\uf471"; // The little icon on the left side of the name, UTF-8 code
    }

    WipSampleHandler::~WipSampleHandler()
    {
        /* Here you'll put stuff that needs to get deleted once the plugin/app is closed.
         * As an example, functions that will free memory, destroy vectors or any other stuff
         * that needs to get "deleted" when you close the plugin/app
         *
         * Like delete[], free(), etc.
         */
    }

    void WipSampleHandler::drawMenu()
    {
        // Here you'll put the stuff that will show up on the left side bar
        if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) // just as an example :)
        {
            ////
        }
    }

    void WipSampleHandler::drawMenuBar()
    {
        // Inside here youn put stuff that will be displayed inside the "Current" tab, at the bar (where you got "File", "Handler", "Current")
        file_open_menu.render("Load JSON File", "Select JSON file", ".", {{"JSON Files", ".json"}});
    }

    void WipSampleHandler::drawContents(ImVec2 win_size)
    {
        // Here you'll put everything that will show up on the "main window", like, anything besides the left side bar where you have options and such
    }
} // namespace satdump
