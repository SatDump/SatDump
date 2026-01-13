#include "widget.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include <filesystem>

#ifdef _MSC_VER
#include <direct.h>
#endif

FileSelectWidget::FileSelectWidget(std::string label, std::string selection_text, bool directory, bool allow_url)
    : label(label), selection_text(selection_text), directory(directory), allow_url(allow_url)
{
    file_select = nullptr;
    dir_select = nullptr; // dirselect = nullptr;
    waiting_for_res = false;
    file_valid = false;
    url_valid = false;
    default_dir = ".";
    id = "##filepathselection" + label;
    btnid = u8"\ufc6e Open##filepathselectionbutton" + label;
}

FileSelectWidget::~FileSelectWidget()
{
    if (file_select != nullptr)
        delete file_select;
    if (dir_select != nullptr)
        delete dir_select; //    delete dirselect;
}

bool FileSelectWidget::draw(std::string hint)
{
    bool changed = false;
    bool disabled = waiting_for_res;

#ifdef _MSC_VER
    if (default_dir == ".")
    {
        char *cwd;
        cwd = _getcwd(NULL, 0);
        if (cwd != 0)
            default_dir = cwd;
    }
#endif

    if (disabled)
        style::beginDisabled();
    if (!file_valid && !url_valid)
        ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);
    if (allow_url)
    {
        ImGuiStyle &style = ImGui::GetStyle();
        float textbox_width =
            ImGui::GetContentRegionAvail().x - (ImGui::CalcTextSize(btnid.c_str(), nullptr, true).x + ImGui::CalcTextSize("Load").x + style.ItemSpacing.x * 2 + style.FramePadding.x * 4);
        if (textbox_width < 20 * ui_scale)
            textbox_width = 20 * ui_scale;
        ImGui::SetNextItemWidth(textbox_width);
    }

    ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &path);
    changed = ImGui::IsItemDeactivatedAfterEdit();
    if (!file_valid && !url_valid)
        ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::Button(btnid.c_str()))
    {
        if (!directory)
        {
            file_select = new fileutils::FileSelTh({{"All Files", "*"}}, default_dir); // TODOREWORK Selection Text?
        }
        else
        {
            dir_select = new fileutils::DirSelTh(default_dir); // TODOREWORK Selection Text?
        }

        waiting_for_res = true;
    }

    if (disabled)
        style::endDisabled();

    if (waiting_for_res)
    {
        std::string get = "";

        bool is_ready = (directory ? dir_select->is_ready() : file_select->is_ready());
        if (is_ready)
        {
            if (!directory)
            {
                get = file_select->result();
                delete file_select;
                file_select = nullptr;
            }

            else
            {
                get = dir_select->result();
                delete dir_select;
                dir_select = nullptr;
            }

            if (get == "")
                waiting_for_res = false;
            else
            {
                path = get;
                changed |= true;
                waiting_for_res = false;
            }
        }
    }

    bool is_dir = std::filesystem::is_directory(path);
    file_valid = std::filesystem::exists(path) && (directory ? is_dir : !is_dir);
    if (allow_url)
    {
        ImGui::SameLine();
        url_valid = path.find("http") == 0;
        if (disabled || (!file_valid && !url_valid))
            style::beginDisabled();
        changed |= ImGui::Button(std::string("Load##" + label).c_str());
        if (disabled || (!file_valid && !url_valid))
            style::endDisabled();
    }
    return (file_valid || url_valid) && changed;
}

std::string FileSelectWidget::getPath() { return path; }

void FileSelectWidget::setPath(std::string new_path) { path = new_path; }

void FileSelectWidget::setDefaultDir(std::string new_path)
{
    default_dir = new_path;
#ifndef _MSC_VER
    if (default_dir.back() != '/')
        default_dir += "/";
#endif
}

bool FileSelectWidget::isValid() { return file_valid; }
