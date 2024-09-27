#pragma once

#include "common/utils.h"
#include "imgui/imgui.h"
#include "downloader.h"
#include "common/widgets/datetime.h"

namespace satdump
{
    class ArchiveLoader
    {
    private:
        bool first_run = true;
        widgets::DateTimePicker request_time;

        widgets::FileDownloaderWidget file_downloader;

        int eumetsat_selected_dataset = -1;
        struct EumetsatElement
        {
            std::string timestamp;
            std::string href;
            std::string size;
        };
        std::vector<EumetsatElement> eumetsat_list;

        void renderEumetsat(ImVec2 wsize);

    public:
        std::string products_download_and_process_directory;

        std::string eumetsat_user_consumer_credential;
        std::string eumetsat_user_consumer_secret;
        std::string getEumetSatToken();

    public:
        ArchiveLoader();
        ~ArchiveLoader();

        void updateEUMETSAT();

        void drawUI(bool *_open);
    };
}