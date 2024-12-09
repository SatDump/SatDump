#include "archive_loader.h"

#include "common/utils.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "processing.h"
#include "main_ui.h"

#include "libs/rapidxml.hpp"

namespace satdump
{
    int goesr_aws_options_SIZE = 4 * 4;
    const std::string goesr_aws_options[] = {
        "GOES-16 Full Disk",
        "GOES-16 CONUS",
        "GOES-16 Mesoscale 1",
        "GOES-16 Mesoscale 2",
        "GOES-17 Full Disk",
        "GOES-17 CONUS",
        "GOES-17 Mesoscale 1",
        "GOES-17 Mesoscale 2",
        "GOES-18 Full Disk",
        "GOES-18 CONUS",
        "GOES-18 Mesoscale 1",
        "GOES-18 Mesoscale 2",
        "GOES-19 Full Disk",
        "GOES-19 CONUS",
        "GOES-19 Mesoscale 1",
        "GOES-19 Mesoscale 2",
    };

    const std::string goesr_aws_options_LUT[][3] = {
        {"goes16", "ABI-L1b-RadF", "RadF"},
        {"goes16", "ABI-L1b-RadC", "RadC"},
        {"goes16", "ABI-L1b-RadM", "RadM1"},
        {"goes16", "ABI-L1b-RadM", "RadM2"},
        {"goes17", "ABI-L1b-RadF", "RadF"},
        {"goes17", "ABI-L1b-RadC", "RadC"},
        {"goes17", "ABI-L1b-RadM", "RadM1"},
        {"goes17", "ABI-L1b-RadM", "RadM2"},
        {"goes18", "ABI-L1b-RadF", "RadF"},
        {"goes18", "ABI-L1b-RadC", "RadC"},
        {"goes18", "ABI-L1b-RadM", "RadM1"},
        {"goes18", "ABI-L1b-RadM", "RadM2"},
        {"goes19", "ABI-L1b-RadF", "RadF"},
        {"goes19", "ABI-L1b-RadC", "RadC"},
        {"goes19", "ABI-L1b-RadM", "RadM1"},
        {"goes19", "ABI-L1b-RadM", "RadM2"},
    };

    void ArchiveLoader::updateGOESRAWS()
    {
        try
        {
            goesr_aws_list.clear();

            std::string satid = goesr_aws_options_LUT[goesr_aws_selected_dataset][0];
            std::string pathid = goesr_aws_options_LUT[goesr_aws_selected_dataset][1];
            std::string fpathid = goesr_aws_options_LUT[goesr_aws_selected_dataset][2];

            int year, dofy;
            {
                time_t tttime = request_time.get();
                std::tm *timeReadable = gmtime(&tttime);
                year = timeReadable->tm_year + 1900;
                dofy = timeReadable->tm_yday + 1;
            }

            std::string url_base = std::string("https://noaa-") + satid + ".s3.amazonaws.com/?list-type=2&prefix=" + pathid + "%2F" + std::to_string(year) + "%2F" + std::to_string(dofy); //%2F15%2F&delimiter=%2F";
            std::string url_req = url_base;
            std::string result;
            while (perform_http_request(url_req, result) != 1)
            {
                printf("\nURL WAS : %s\n", url_req.c_str());

                rapidxml::xml_document<> doc;
                doc.parse<0>((char *)result.c_str());

                if (doc.first_node("ListBucketResult") == 0)
                    throw satdump_exception("XML missing ListBucketResult!");

                for (rapidxml::xml_node<> *content_node = doc.first_node("ListBucketResult")->first_node("Contents"); content_node; content_node = content_node->next_sibling())
                {

                    std::string path = content_node->first_node("Key")->value();
                    std::string stem = std::filesystem::path(path).stem().string();

                    time_t timestamp = 0;
                    int mode = -1;
                    int channel = -1;
                    {
                        std::tm timeS;
                        int day_of_year = 0;
                        memset(&timeS, 0, sizeof(std::tm));
                        if (sscanf(stem.c_str(),
                                   std::string("OR_ABI-L1b-" + fpathid + "-M%1dC%2d_G%*d_s%4d%3d%2d%2d%2d%*d_e%*llu_c%*llu").c_str(),
                                   &mode, &channel,
                                   &timeS.tm_year, &day_of_year, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 7)
                        {
                            timeS.tm_year -= 1900;
                            timestamp = timegm(&timeS) + day_of_year * 3600 * 24;
                        }
                    }

                    std::string timestampstr = timestamp_to_string(timestamp);
                    //   printf("New File %s - Channel %d\n", timestampstr.c_str(), channel);

                    bool existed = false;
                    for (auto &v : goesr_aws_list)
                    {
                        if (v.timestamp == timestampstr)
                        {
                            v.channels.insert_or_assign(channel, "https://noaa-" + satid + ".s3.amazonaws.com/" + path);
                            existed = true;
                            break;
                        }
                    }
                    if (!existed)
                        goesr_aws_list.push_back({timestamp, timestampstr, {{channel, "https://noaa-" + satid + ".s3.amazonaws.com/" + path}}});
                }

                // Continuation
                if (doc.first_node("ListBucketResult")->first_node("NextContinuationToken") != nullptr)
                {
                    std::string next_token = doc.first_node("ListBucketResult")->first_node("NextContinuationToken")->value();
                    logger->trace("Continuation token : " + next_token);

                    CURL *curl = curl_easy_init();
                    if (curl)
                    {
                        char *output = curl_easy_escape(curl, next_token.c_str(), next_token.length());
                        if (output)
                        {
                            url_req = url_base + "&continuation-token=" + std::string(output);
                            curl_free(output);
                        }
                        else
                            break;
                        curl_easy_cleanup(curl);
                    }
                    else
                        break;

                    result.clear();
                    continue;
                }

                break;
            }
        }
        catch (std::exception &e)
        {
            logger->error("Error updating FD list! %s", e.what());
        }
    }

    void ArchiveLoader::renderGOESRAWS(ImVec2 wsize)
    {
        bool should_disable = file_downloader.is_busy() || satdump::processing::is_processing;

        if (should_disable)
            style::beginDisabled();

        if (ImGui::BeginCombo("##archiveloader_dataset", goesr_aws_options[goesr_aws_selected_dataset].c_str()))
        {
            for (int i = 0; i < goesr_aws_options_SIZE; i++)
                if (ImGui::Selectable(goesr_aws_options[i].c_str(), goesr_aws_selected_dataset == i))
                {
                    goesr_aws_selected_dataset = i;
                    updateGOESRAWS();
                }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh##archiveloader_refresh"))
            updateGOESRAWS();

        request_time.draw();
        ImGui::SameLine();
        if (ImGui::Button("Current##archiveloader_setcurrenttime"))
            request_time.set(time(0));

        ImGui::BeginChild("##archiveloader_subwindow", {wsize.x - 50 * ui_scale, wsize.y - 190 * ui_scale}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (ImGui::BeginTable("##archiveloadertable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("##archiveloadertable_name", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("##archiveloadertable_butt", ImGuiTableColumnFlags_None);

            for (auto &str : goesr_aws_list)
            {
                ImGui::TableNextColumn();
                ImGui::Text("%s", str.timestamp.c_str());

                ImGui::TableNextColumn();
                if (ImGui::Button(std::string("Load##archiveloadertablebutton_" + str.timestamp).c_str()))
                {
                    auto func = [this, str](int)
                    {
                        try
                        {
                            std::string l_download_path;
                            bool success = true;
                            for (auto &c : str.channels)
                            {
                                std::string download_path = products_download_and_process_directory + "/" +
                                                            std::filesystem::path(c.second).stem().string() +
                                                            std::filesystem::path(c.second).extension().string();

                                if (file_downloader.download_file(c.second, download_path, "") == 1)
                                {
                                    success = false;
                                    break;
                                }

                                l_download_path = download_path;
                            }

                            std::string process_path = products_download_and_process_directory + "/" + std::filesystem::path(l_download_path).stem().string();

                            if (success)
                                processing::process("off2pro",
                                                    "file",
                                                    l_download_path,
                                                    process_path,
                                                    {});
                        }
                        catch (std::exception &e)
                        {
                            logger->error("Failed downloading file from GOES-R AWS! %s", e.what());
                        }
                    };

                    ui_thread_pool.push(func);
                }
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();
        if (should_disable)
            style::endDisabled();

        file_downloader.render();
    }
}