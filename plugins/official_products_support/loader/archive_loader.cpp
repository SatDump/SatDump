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
        if (ImGui::BeginTabItem("GOES-R AWS"))
        {
            renderGOESRAWS(wsize);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("EUMETSAT"))
        {
            renderEumetsat(wsize);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        ImGui::End();
    }
}