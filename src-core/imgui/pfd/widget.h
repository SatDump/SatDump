#pragma once

#include "imgui/imgui.h"
#include <filesystem>
#include "portable-file-dialogs.h"
#include "imgui/imgui_stdlib.h"
#include "android_dialogs.h"

struct FileSelectWidget
{
    std::string label;
    std::string selection_text;
    std::string id;
    std::string btnid;

    std::string path;
    bool file_valid;

    bool directory;

#ifdef __ANDROID__
    bool waiting_for_res = false;
#endif

    bool draw(std::string hint = "")
    {
        bool changed = false;

        bool is_dir = std::filesystem::is_directory(path);
        file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);

        if (!file_valid)
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        changed |= ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &path);
        if (!file_valid)
            ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button(btnid.c_str()))
        {
            if (!directory)
            {
#ifdef __ANDROID__
                show_select_file_dialog();
#else
                auto fileselect = pfd::open_file(selection_text.c_str(), ".", {});

                while (!fileselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (fileselect.result().size() > 0)
                    path = fileselect.result()[0];
#endif
            }
            else
            {
#ifdef __ANDROID__
                show_select_directory_dialog();
#else
                auto dirselect = pfd::select_folder(selection_text.c_str(), ".");

                while (!dirselect.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (dirselect.result().size() > 0)
                    path = dirselect.result();
#endif
            }

            changed = true;
#ifdef __ANDROID__
            waiting_for_res = true;
#endif
            file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);
        }

#ifdef __ANDROID__
        if (waiting_for_res)
        {
            std::string get;
            if (!directory)
                get = get_select_file_dialog_result();
            else
                get = get_select_directory_dialog_result();
            if (get != "")
            {
                path = get;
                changed = true;
                file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);
                waiting_for_res = false;
            }
        }
#endif

        return file_valid && changed;
    }

    FileSelectWidget(std::string label, std::string selection_text, bool directory = false)
        : label(label), selection_text(selection_text), directory(directory)
    {
        id = "##filepathselection" + label;
        btnid = u8"\ufc6e Open##filepathselectionbutton" + label;
    }

    std::string getPath()
    {
        return path;
    }
};