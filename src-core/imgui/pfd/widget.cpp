#include <filesystem>
#include "widget.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "android_dialogs.h"

#ifdef _MSC_VER
#include <direct.h>
#endif

FileSelectWidget::FileSelectWidget(std::string label, std::string selection_text, bool directory)
    : label(label), selection_text(selection_text), directory(directory)
{
    fileselect = nullptr;
    dirselect = nullptr;
    waiting_for_res = false;
    default_dir = ".";
    id = "##filepathselection" + label;
    btnid = u8"\ufc6e Open##filepathselectionbutton" + label;
}

FileSelectWidget::~FileSelectWidget()
{
    delete fileselect;
    delete dirselect;
}

bool FileSelectWidget::draw(std::string hint)
{
    bool changed = false;
    bool disabled = waiting_for_res;

#ifdef _MSC_VER
    if (default_dir == ".")
    {
        char* cwd;
        cwd = _getcwd(NULL, 0);
        if (cwd != 0)
            default_dir = cwd;
    }
#endif
    if (disabled)
        style::beginDisabled();
    if (!file_valid)
        ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);
    ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &path);
    changed = ImGui::IsItemDeactivatedAfterEdit();
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
            fileselect = new pfd::open_file(selection_text.c_str(), default_dir, { "All Files", "*" }, pfd::opt::force_path);
#endif
        }
        else
        {
#ifdef __ANDROID__
            show_select_directory_dialog();
#else
            dirselect = new pfd::select_folder(selection_text.c_str(), default_dir, pfd::opt::force_path);
#endif
        }

        waiting_for_res = true;
    }
    if (disabled)
        style::endDisabled();

    if (waiting_for_res)
    {
        std::string get = "";
#ifdef __ANDROID__
        if (!directory)
            get = get_select_file_dialog_result();
        else
            get = get_select_directory_dialog_result();
        if (get != "")
        {
            if(get == "NO_PATH_SELECTED")
                waiting_for_res = false;
            else
            {
#else
        bool is_ready = (directory ? dirselect->ready(0) : fileselect->ready(0));
        if (is_ready)
        {
            if (!directory)
            {
                get = (fileselect->result().size() == 0 ? "" :  fileselect->result()[0]);
                delete fileselect;
                fileselect = nullptr;
            }
                
            else
            {
                get = dirselect->result();
                delete dirselect;
                dirselect = nullptr;
            }

            if (get == "")
                waiting_for_res = false;
            else
            {
#endif
                path = get;
                changed = true;
                waiting_for_res = false;
            }
        }
    }

    bool is_dir = std::filesystem::is_directory(path);
    file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);
    return file_valid && changed;
}

std::string FileSelectWidget::getPath()
{
    return path;
}

void FileSelectWidget::setPath(std::string new_path)
{
    path = new_path;
}

void FileSelectWidget::setDefaultDir(std::string new_path)
{
    default_dir = new_path;
#ifndef _MSC_VER
    if(default_dir.back() != '/')
        default_dir += "/";
#endif
}

bool FileSelectWidget::isValid()
{
    return file_valid;
}
