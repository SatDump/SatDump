#pragma once

#include "common/utils.h"
#include "imgui/imgui.h"
#include "downloader.h"
#include "common/widgets/datetime.h"
#include "imgui/pfd/widget.h"

namespace satdump
{
    class ArchiveLoader
    {
    private:
        widgets::DateTimePicker request_time;
        widgets::FileDownloaderWidget file_downloader;

    private: // AWS
        struct AwsOptions
        {
            std::string name;
            std::string satid;
            std::string pathid;
            void (ArchiveLoader::* updateFunc)();
        };

        const std::vector<AwsOptions> aws_options = {
            {"GOES-16 Full Disk", "goes16", "ABI-L1b-RadF", &ArchiveLoader::updateGOESRAWS},
            {"GOES-16 CONUS", "goes16", "ABI-L1b-RadC", &ArchiveLoader::updateGOESRAWS},
            {"GOES-16 Mesoscale 1", "goes16", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GOES-16 Mesoscale 2", "goes16", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GOES-17 Full Disk", "goes17", "ABI-L1b-RadF", &ArchiveLoader::updateGOESRAWS},
            {"GOES-17 PACUS", "goes17", "ABI-L1b-RadC", &ArchiveLoader::updateGOESRAWS},
            {"GOES-17 Mesoscale 1", "goes17", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GOES-17 Mesoscale 2", "goes17", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GOES-18 Full Disk", "goes18", "ABI-L1b-RadF", &ArchiveLoader::updateGOESRAWS},
            {"GOES-18 PACUS", "goes18", "ABI-L1b-RadC", &ArchiveLoader::updateGOESRAWS},
            {"GOES-18 Mesoscale 1", "goes18", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GOES-18 Mesoscale 2", "goes18", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GOES-19 Full Disk", "goes19", "ABI-L1b-RadF", &ArchiveLoader::updateGOESRAWS},
            {"GOES-19 CONUS", "goes19", "ABI-L1b-RadC", &ArchiveLoader::updateGOESRAWS},
            {"GOES-19 Mesoscale 1", "goes19", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GOES-19 Mesoscale 2", "goes19", "ABI-L1b-RadM", &ArchiveLoader::updateGOESRAWS},
            {"GK-2A Full Disk", "gk2a-pds", "FD", &ArchiveLoader::updateGK2AAWS},
            {"GK-2A Local Area", "gk2a-pds", "LA", &ArchiveLoader::updateGK2AAWS},
            {"Himawari-9 Full Disk", "himawari9", "AHI-L1b-FLDK", &ArchiveLoader::updateHimawariAWS},
            {"Himawari-9 Japan", "himawari9", "AHI-L1b-Japan", &ArchiveLoader::updateHimawariAWS},
            {"Himawari-9 Target", "himawari9", "AHI-L1b-Target", &ArchiveLoader::updateHimawariAWS}
        };

        int aws_selected_dataset = 0;
        std::map<std::string, std::set<std::string>> aws_list; //Timestamp, URL Set

        void renderAWS(ImVec2 wsize);
        void queryAWS(std::string url_host, std::string url_path, void (ArchiveLoader::* parseTimestamp)(std::string, time_t &, std::string &));

        void parseGOESTimestamp(std::string filename, time_t &timestamp, std::string &channel);
        void updateGOESRAWS();

        void parseGK2ATimestamp(std::string filename, time_t &timestamp, std::string &channel);
        void updateGK2AAWS();

        void parseHimawariTimestamp(std::string filename, time_t &timestamp, std::string &channel);
        void updateHimawariAWS();

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

    private: // General
        std::string products_download_and_process_directory;
        int download_location = 0;
        FileSelectWidget output_selection = FileSelectWidget("downloaderoutputdir", "Output Directory", true);

    public:
        std::string eumetsat_user_consumer_credential;
        std::string eumetsat_user_consumer_secret;
        std::string getEumetSatToken();

    public:
        ArchiveLoader();
        ~ArchiveLoader();

        void drawUI(bool *_open);
    };
}