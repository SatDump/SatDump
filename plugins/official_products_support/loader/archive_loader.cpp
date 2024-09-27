#include "archive_loader.h"
#include <filesystem>
#include "core/module.h"

namespace satdump
{
    ArchiveLoader::ArchiveLoader()
        : request_time("###archiveloeaderequesttime", time(0))
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
        if (first_run)
        {
            ImGui::SetNextWindowSize({500 * ui_scale, 500 * ui_scale});
            first_run = false;
        }

        ImGui::Begin("Archive Loader", _open);

        ImGui::BeginTabBar("##archiveloadertabbar");
        if (ImGui::BeginTabItem("EUMETSAT"))
        {
            ImVec2 wsize = ImGui::GetWindowSize();
            renderEumetsat(wsize);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        ImGui::End();
    }
}