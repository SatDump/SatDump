#include "archive_loader.h"
#include <filesystem>

namespace satdump
{

    ArchiveLoader::ArchiveLoader()
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