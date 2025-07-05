#pragma once

#include "common/widgets/menuitem_fileopen.h"
#include "handlers/handler.h"
#include "imgui/imgui.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>


// order-keeping JSON alias
using ordered_json = nlohmann::ordered_json;

namespace satdump
{
    class WipSettingsEditorHandler : public handlers::Handler
    {
    protected:
        void drawMenu();
        void drawContents(ImVec2 win_size);
        void drawMenuBar();

    private:
        // File open dialog
        widget::MenuItemFileOpen file_open_menu;

        // Internal state storage for instruments and presets
        std::vector<std::string> json_files;
        std::vector<std::vector<std::string>> preset_names;
        std::vector<std::vector<bool>>         preset_selected;

        std::vector<ordered_json>              original_jsons;

        std::vector<std::string> all_instruments;
        std::vector<std::string> enabled_instruments;

        int instrument_to_add = -1;
        int selected_dropdown = -1;

        // Export
        char       export_dir[512] = "";
        bool       export_success  = false;
        std::string export_status_msg;

        bool export_config_pressed = false;

    public:
        WipSettingsEditorHandler();
        ~WipSettingsEditorHandler();

        std::string getID()   { return "wipsettings_handler"; }
        std::string getName() { return "Instrument Config Generator"; }
    };
} // namespace satdump
