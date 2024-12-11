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
        widgets::DateTimePicker request_time;
        widgets::FileDownloaderWidget file_downloader;

    private: // GOES-R AWS
        int goesr_aws_selected_dataset = 0;
        struct GoesAwsElement
        {
            time_t tim;
            std::string timestamp;
            std::map<int, std::string> channels;
        };
        std::vector<GoesAwsElement> goesr_aws_list;

        void renderGOESRAWS(ImVec2 wsize);
        void updateGOESRAWS();

    private: // EUMETSAT
        int eumetsat_selected_dataset = 0;
        struct EumetsatElement
        {
            std::string timestamp;
            std::string href;
            std::string size;
        };
        std::vector<EumetsatElement> eumetsat_list;

        void renderEumetsat(ImVec2 wsize);
        void updateEUMETSAT();

    public:
        std::string products_download_and_process_directory;

        std::string eumetsat_user_consumer_credential;
        std::string eumetsat_user_consumer_secret;
        std::string getEumetSatToken();

    public:
        ArchiveLoader();
        ~ArchiveLoader();

        void drawUI(bool *_open);
    };
}