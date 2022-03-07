#pragma once

#include "imgui/imgui.h"
#include <filesystem>
#include "portable-file-dialogs.h"

struct FileSelectWidget
{
    std::string label;
    std::string selection_text;
    std::string id;
    std::string btnid;

    char path[10000];
    bool file_valid;

    bool directory;

    void draw()
    {
        bool is_dir = std::filesystem::is_directory(path);
        file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);

        if (!file_valid)
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        ImGui::InputText(id.c_str(), path, 10000);
        if (!file_valid)
            ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button(btnid.c_str()))
        {
            if (!directory)
            {
                auto fileselect = pfd::open_file(selection_text.c_str(), ".", {}, false);

                while (!fileselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (fileselect.result().size() > 0)
                {
                    memset(path, 0, 10000);
                    memcpy(path, fileselect.result()[0].c_str(), fileselect.result()[0].size());
                }
            }
            else
            {
                auto dirselect = pfd::select_folder(selection_text.c_str(), ".");

                while (!dirselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (dirselect.result().size() > 0)
                {
                    memset(path, 0, 10000);
                    memcpy(path, dirselect.result().c_str(), dirselect.result().size());
                }
            }
        }
    }

    FileSelectWidget(std::string label, std::string selection_text, bool directory = false)
        : label(label), selection_text(selection_text), directory(directory)
    {
        id = "##filepathselection" + label;
        btnid = "Select##filepathselectionbutton" + label;
    }

    std::string getPath()
    {
        return std::string(path);
    }
};