#pragma once

#include "imgui/imgui.h"
#include <filesystem>
#include "portable-file-dialogs.h"
#include "imgui/imgui_stdlib.h"

struct FileSelectWidget
{
    std::string label;
    std::string selection_text;
    std::string id;
    std::string btnid;

    std::string path;
    bool file_valid;

    bool directory;

    bool draw()
    {
        bool changed = false;

        bool is_dir = std::filesystem::is_directory(path);
        file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);

        if (!file_valid)
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        changed |= ImGui::InputText(id.c_str(), &path);
        if (!file_valid)
            ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button(btnid.c_str()))
        {
            if (!directory)
            {
                auto fileselect = pfd::open_file(selection_text.c_str(), ".", {});

                while (!fileselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (fileselect.result().size() > 0)
                    path = fileselect.result()[0];
            }
            else
            {
                auto dirselect = pfd::select_folder(selection_text.c_str(), ".");

                while (!dirselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (dirselect.result().size() > 0)
                    path = dirselect.result();
            }

            changed = true;
            file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);
        }

        return file_valid && changed;
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