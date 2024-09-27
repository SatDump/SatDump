#pragma once

#include "common/utils.h"
#include "imgui/imgui.h"
#include "downloader.h"

namespace satdump
{
    class ArchiveLoader
    {
    private:
        widgets::FileDownloaderWidget file_downloader;

        int eumetsat_selected_dataset = -1;
        std::vector<std::pair<std::string, std::string>> eumetsat_list;

        void renderEumetsat(ImVec2 wsize);

    public:
        std::string products_download_and_process_directory;

        std::string eumetsat_user_consumer_token;
        std::string getEumetSatToken();

    public:
        ArchiveLoader();
        ~ArchiveLoader();

        void updateEUMETSAT();

        void drawUI(bool *_open);
    };
}