#include "archive_loader.h"
#include <filesystem>
#include "core/style.h"

namespace satdump
{
    ArchiveLoader::ArchiveLoader()
        : request_time("###archiveloeaderequesttime", time(0), true)
    {
        products_download_and_process_directory = std::filesystem::temp_directory_path().string() + "/satdump_official";
        if (!std::filesystem::exists(products_download_and_process_directory))
            std::filesystem::create_directories(products_download_and_process_directory);
    }

    ArchiveLoader::~ArchiveLoader()
    {
    }

    void ArchiveLoader::drawUI(bool *_open)
    {
        ImGui::SetNextWindowSize({500 * ui_scale, 500 * ui_scale}, ImGuiCond_Once);
        ImGui::Begin("Archive Loader", _open);

        ImGui::BeginTabBar("##archiveloadertabbar");
        ImVec2 wsize = ImGui::GetWindowSize();
        if (ImGui::BeginTabItem("NOAA AWS"))
        {
            renderAWS(wsize);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("EUMETSAT"))
        {
            renderEumetsat(wsize);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        file_downloader.render();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginChild("##archiveloaderoutputselect", { ImGui::GetWindowContentRegionMax().x, 0 }, false, ImGuiWindowFlags_NoScrollbar))
        {
            bool disable_options = file_downloader.is_busy();
            if (disable_options)
                style::beginDisabled();

            ImGui::TextUnformatted("Decode To:");
            ImGui::SameLine();
            ImGui::RadioButton("Temporary", &download_location, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Selected Folder:", &download_location, 1);

            if (download_location == 0)
                style::beginDisabled();
            output_selection.draw();
            if (download_location == 0)
                style::endDisabled();

            if (disable_options)
                style::endDisabled();
        }
        
        ImGui::EndChild();
        ImGui::End();
    }
}