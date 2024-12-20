#include "archive_loader.h"

#include "common/utils.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "processing.h"
#include "main_ui.h"

#include "libs/rapidxml.hpp"

namespace satdump
{
    void ArchiveLoader::queryAWS(std::string url_host, std::string url_path, void (ArchiveLoader::* parseTimestamp)(std::string, time_t &, std::string &))
    {
        std::string url_req = url_host + url_path;
        std::string result;
        while (perform_http_request(url_req, result) != 1)
        {
            // logger->trace("\nURL WAS : %s\n", url_req.c_str());

            rapidxml::xml_document<> doc;
            doc.parse<0>((char*)result.c_str());

            if (doc.first_node("ListBucketResult") == 0)
                throw satdump_exception("XML missing ListBucketResult!");

            for (rapidxml::xml_node<> *content_node = doc.first_node("ListBucketResult")->first_node("Contents"); content_node; content_node = content_node->next_sibling())
            {

                std::string path = content_node->first_node("Key")->value();
                std::string stem = std::filesystem::path(path).stem().string();

                time_t timestamp = 0;
                std::string channel;
                (this->*parseTimestamp)(stem, timestamp, channel);
                aws_list[timestamp_to_string(timestamp)].insert(url_host + path);
            }

            // Continuation
            if (doc.first_node("ListBucketResult")->first_node("NextContinuationToken") != nullptr)
            {
                std::string next_token = doc.first_node("ListBucketResult")->first_node("NextContinuationToken")->value();
                logger->trace("Continuation token : " + next_token);

                CURL* curl = curl_easy_init();
                if (curl)
                {
                    char* output = curl_easy_escape(curl, next_token.c_str(), next_token.length());
                    if (output)
                    {
                        url_req = url_host + url_path + "&continuation-token=" + std::string(output);
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

    void ArchiveLoader::parseGOESTimestamp(std::string filename, time_t &timestamp, std::string &channel)
    {
        std::tm timeS;
        int day_of_year = 0;
        int channel_int = 0;
        memset(&timeS, 0, sizeof(std::tm));
        if (sscanf(filename.c_str(), "OR_ABI-L1b-Rad%*[FCM12]-M%*dC%2d_G%*d_s%4d%3d%2d%2d%2d%*d_e%*llu_c%*llu",
            &channel_int, &timeS.tm_year, &day_of_year, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
        {
            channel = std::to_string(channel_int);
            timeS.tm_year -= 1900;
            timestamp = timegm(&timeS) + day_of_year * 3600 * 24;
        }
    }

    void ArchiveLoader::updateGOESRAWS()
    {
        try
        {
            aws_list.clear();
            int year, dofy;
            {
                time_t tttime = request_time.get();
                std::tm *timeReadable = gmtime(&tttime);
                year = timeReadable->tm_year + 1900;
                dofy = timeReadable->tm_yday + 1;
            }

            queryAWS(std::string("https://noaa-") + aws_options[aws_selected_dataset].satid + ".s3.amazonaws.com/",
                std::string("?list-type=2&prefix=") + aws_options[aws_selected_dataset].pathid + "%2F" + std::to_string(year) + "%2F" +
                std::to_string(dofy), &ArchiveLoader::parseGOESTimestamp);
        }
        catch (std::exception &e)
        {
            logger->error("Error updating GOES Sector list! %s", e.what());
        }
    }

    void ArchiveLoader::parseGK2ATimestamp(std::string filename, time_t &timestamp, std::string &channel)
    {
        std::tm timeS;
        char channel_char[6] = { 0 };
        memset(&timeS, 0, sizeof(std::tm));
        if (sscanf(filename.c_str(), "gk2a_ami_le1b_%5c_%*5cge_%4d%2d%2d%2d%2d",
            channel_char, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min) == 6)
        {
            channel = std::string(channel_char);
            timeS.tm_year -= 1900;
            timeS.tm_mon--;

            timestamp = timegm(&timeS);
        }
    }

    void ArchiveLoader::updateGK2AAWS()
    {
        try
        {
            aws_list.clear();

            std::string yearmonth, date;
            {
                std::stringstream string_stream;
                time_t tttime = request_time.get();
                struct tm timeinfo_struct;
#ifdef _WIN32
                memcpy(&timeinfo_struct, gmtime(&tttime), sizeof(struct tm));
#else
                gmtime_r(&tttime, &timeinfo_struct);
#endif

                string_stream << timeinfo_struct.tm_year + 1900 << std::setfill('0') << std::setw(2) << timeinfo_struct.tm_mon + 1;
                yearmonth = string_stream.str();
                std::stringstream().swap(string_stream);
                string_stream << std::setfill('0') << std::setw(2) << timeinfo_struct.tm_mday;
                date = string_stream.str();
                std::stringstream().swap(string_stream);
                string_stream << std::setfill('0') << std::setw(2) << timeinfo_struct.tm_hour;
            }

            queryAWS(std::string("https://noaa-" + aws_options[aws_selected_dataset].satid + ".s3.amazonaws.com/"),
                std::string("?list-type=2&prefix=AMI%2FL1B%2F" + aws_options[aws_selected_dataset].pathid +
                "%2F" + yearmonth + "%2F" + date), &ArchiveLoader::parseGK2ATimestamp);
        }
        catch (std::exception &e)
        {
            logger->error("Error updating GK-2A Sector list! %s", e.what());
        }
    }

    void ArchiveLoader::parseHimawariTimestamp(std::string filename, time_t &timestamp, std::string &channel)
    {
        std::tm timeS;
        int channel_int;

        char subtime[3] = { 0 };
        char region[3] = { 0 };

        memset(&timeS, 0, sizeof(std::tm));
        if (sscanf(filename.c_str(), "HS_H%*d_%4d%2d%2d_%2d%2d_B%2d_%2c%2c_R%*2d_S%*4d.DAT",
            &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &channel_int, region, subtime) == 8)
        {
            channel = std::to_string(channel_int);
            timeS.tm_year -= 1900;
            timeS.tm_mon--;
            timestamp = timegm(&timeS);

            // Himawari does not have the actual time listed in the file name - it is rounded to the last 10 minutes
            // Estimate the correct time
            int subtime_int = strtol(subtime, nullptr, 10);
            if (subtime_int != 0)
            {
                if (region[0] == 'J')
                    timestamp += (subtime_int - 1) * 150;
                else
                    timestamp += subtime_int * 150;
            }
        }
    }

    void ArchiveLoader::updateHimawariAWS()
    {
        try
        {
            aws_list.clear();

            std::string year, month, date;
            std::stringstream string_stream;
            {
                time_t tttime = request_time.get();
                struct tm timeinfo_struct;
#ifdef _WIN32
                memcpy(&timeinfo_struct, gmtime(&tttime), sizeof(struct tm));
#else
                gmtime_r(&tttime, &timeinfo_struct);
#endif

                year = std::to_string(timeinfo_struct.tm_year + 1900);
                string_stream << std::setfill('0') << std::setw(2) << timeinfo_struct.tm_mon + 1;
                month = string_stream.str();
                std::stringstream().swap(string_stream);
                string_stream << std::setfill('0') << std::setw(2) << timeinfo_struct.tm_mday;
                date = string_stream.str();
                std::stringstream().swap(string_stream);
            }

            queryAWS(std::string("https://noaa-" + aws_options[aws_selected_dataset].satid + ".s3.amazonaws.com/"),
                std::string("?list-type=2&prefix=" + aws_options[aws_selected_dataset].pathid + "%2F" + year + "%2F" + month + "%2F" + date),
                &ArchiveLoader::parseHimawariTimestamp);
        }
        catch (std::exception& e)
        {
            logger->error("Error updating Himawari Sector list! %s", e.what());
        }
    }

    void ArchiveLoader::renderAWS(ImVec2 wsize)
    {
        bool should_disable = file_downloader.is_busy() || satdump::processing::is_processing;

        if (should_disable)
            style::beginDisabled();

        if (ImGui::BeginCombo("##archiveloader_dataset", aws_options[aws_selected_dataset].name.c_str()))
        {
            for (int i = 0; i < (int)aws_options.size(); i++)
            {
                if (ImGui::Selectable(aws_options[i].name.c_str(), aws_selected_dataset == i))
                {
                    aws_selected_dataset = i;
                    (this->*(aws_options[aws_selected_dataset].updateFunc))();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh##archiveloader_refresh"))
            (this->*(aws_options[aws_selected_dataset].updateFunc))();

        ImGui::TextUnformatted("Date: ");
        ImGui::SameLine();
        request_time.draw();
        ImGui::SameLine();
        if (ImGui::Button("Current##archiveloader_setcurrenttime"))
            request_time.set(time(0));

        float target_height = wsize.y - 260 * ui_scale;
        ImGui::BeginChild("##archiveloader_subwindow", { ImGui::GetContentRegionAvail().x, target_height < 5 * ui_scale ? 5 * ui_scale : target_height },
            false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        if (ImGui::BeginTable("##archiveloadertable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("##archiveloadertable_name", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("##archiveloadertable_butt", ImGuiTableColumnFlags_None);

            for (auto &str : aws_list)
            {
                ImGui::TableNextColumn();
                ImGui::Text("%s", str.first.c_str());

                ImGui::TableNextColumn();
                if (ImGui::Button(std::string("Load##archiveloadertablebutton_" + str.first).c_str()))
                {
                    auto func = [this, str](int)
                    {
                        try
                        {
                            std::string l_download_path;
                            bool success = true;
                            for (auto &c : str.second)
                            {
                                std::string download_path = products_download_and_process_directory + "/" +
                                    std::filesystem::path(c).stem().string() +
                                    std::filesystem::path(c).extension().string();

                                if (file_downloader.download_file(c, download_path, "") == 1)
                                {
                                    success = false;
                                    break;
                                }

                                l_download_path = download_path;
                            }

                            std::string process_path = (download_location && output_selection.isValid() ?
                                output_selection.getPath() : products_download_and_process_directory) +
                                "/" + std::filesystem::path(l_download_path).stem().string();

                            if (success)
                                processing::process("off2pro",
                                                    "file",
                                                    l_download_path,
                                                    process_path,
                                                    {});
                        }
                        catch (std::exception &e)
                        {
                            logger->error("Failed downloading file from AWS! %s", e.what());
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
    }
}