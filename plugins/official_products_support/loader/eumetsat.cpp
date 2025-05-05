#include "archive_loader.h"

#include "common/utils.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "processing.h"
#include "main_ui.h"
#include "libs/base64/base64.h"

namespace satdump
{
    struct EumetSatProductItem
    {
        std::string name;
        std::string id;
        bool is_hdf;
    };

    std::vector<EumetSatProductItem> eumetsat_products = {
        {
            "MTG FCI 0 deg Normal Resolution",
            "EO%3AEUM%3ADAT%3A0662",
            true,
        },
        {
            "MTG FCI 0 deg Full Resolution",
            "EO%3AEUM%3ADAT%3A0665",
            true,
        },
        {
            "MSG SEVIRI 0 deg",
            "EO%3AEUM%3ADAT%3AMSG%3AHRSEVIRI",
            false,
        },
        {
            "MSG SEVIRI 0 deg RSS",
            "EO%3AEUM%3ADAT%3AMSG%3AMSG15-RSS",
            false,
        },
        {
            "MSG SEVIRI IODC",
            "EO%3AEUM%3ADAT%3AMSG%3AHRSEVIRI-IODC",
            false,
        },
        {
            "MetOp AVHRR",
            "EO%3AEUM%3ADAT%3AMETOP%3AAVHRRL1",
            false,
        },
        {
            "MetOp MHS",
            "EO%3AEUM%3ADAT%3AMETOP%3AMHSL1",
            false,
        },
        {
            "MetOp AMSU",
            "EO%3AEUM%3ADAT%3AMETOP%3AAMSUL1",
            false,
        },
        {
            "MetOp HIRS",
            "EO%3AEUM%3ADAT%3AMETOP%3AHIRSL1",
            false,
        },
        {
            "Sentinel-3 OLCI Full Resolution",
            "EO%3AEUM%3ADAT%3A0409",
            true,
        },
        {
            "Sentinel-3 SLSTR",
            "EO%3AEUM%3ADAT%3A0411",
            true,
        },
    };

    std::string ArchiveLoader::getEumetSatToken()
    {
        std::string final_token = macaron::Base64::Encode(eumetsat_user_consumer_credential + ":" + eumetsat_user_consumer_secret);

        std::string resp = "";
        if (perform_http_request_post("https://api.eumetsat.int/token", resp, "grant_type=client_credentials", "Authorization: Basic " + final_token) != 1)
            resp = nlohmann::json::parse(resp)["access_token"];
        logger->trace("Token " + resp);
        return resp;
    }

    void ArchiveLoader::updateEUMETSAT()
    {
        try
        {
            int year, month, day;
            {
                time_t tttime = request_time.get();
                std::tm *timeReadable = gmtime(&tttime);
                year = timeReadable->tm_year + 1900;
                month = timeReadable->tm_mon + 1;
                day = timeReadable->tm_mday;
            }

            std::string url = "https://api.eumetsat.int/data/browse/1.0.0/collections/" + std::string(eumetsat_products[eumetsat_selected_dataset].id) + "/dates/" +
                              std::to_string(year) + "/" +
                              (month < 10 ? "0" : "") + std::to_string(month) + "/" +
                              (day < 10 ? "0" : "") + std::to_string(day) +
                              "/products?format=json";
            std::string resp;
            logger->info(url);
            if (perform_http_request(url, resp, "") != 1)
            {
                eumetsat_list.clear();

                nlohmann::json respj = nlohmann::json::parse(resp);
                //            saveJsonFile("test.json", respj);
                if (respj.contains("products"))
                {
                    for (size_t i = 0; i < respj["products"].size(); i++)
                    {
                        auto &prod = respj["products"][i];

                        std::tm timeS;
                        memset(&timeS, 0, sizeof(std::tm));

                        if (sscanf(prod["date"].get<std::string>().c_str(),
                                   "%4d-%2d-%2dT%2d:%2d:%2d.%*dZ/%*d-%*d-%*dT%*d:%*d:%*d.%*dZ",
                                   &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                                   &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
                        {
                            timeS.tm_year -= 1900;
                            timeS.tm_mon -= 1;
                            timeS.tm_isdst = -1;
                            time_t timestamp = timegm(&timeS);

                            std::string prod_d = prod["links"][0]["href"];
                            eumetsat_list.push_back({timestamp_to_string(timestamp), prod_d});
                        }
                    }
                }
            }
        }
        catch (std::exception &e)
        {
            logger->error("Error updating product list! %s", e.what());
        }
    }

    void ArchiveLoader::renderEumetsat(ImVec2 wsize)
    {
        bool should_disable = file_downloader.is_busy() || satdump::processing::is_processing;

        if (should_disable)
            style::beginDisabled();

        if (ImGui::BeginCombo("##archiveloader_dataset", eumetsat_products[eumetsat_selected_dataset].name.c_str()))
        {
            for (int i = 0; i < (int)eumetsat_products.size(); i++)
                if (ImGui::Selectable(eumetsat_products[i].name.c_str(), eumetsat_selected_dataset == i))
                {
                    eumetsat_selected_dataset = i;
                    updateEUMETSAT();
                }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh##archiveloader_refresh"))
            updateEUMETSAT();

        ImGui::TextUnformatted("Date: ");
        ImGui::SameLine();
        request_time.draw();
        ImGui::SameLine();
        if (ImGui::Button("Current##archiveloader_setcurrenttime"))
            request_time.set(time(0));

        float target_height = wsize.y - 260 * ui_scale;
        ImGui::BeginChild("##archiveloader_subwindow", {ImGui::GetContentRegionAvail().x, target_height < 5 * ui_scale ? 5 * ui_scale : target_height },
            false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (ImGui::BeginTable("##archiveloadertable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("##archiveloadertable_name", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("##archiveloadertable_butt", ImGuiTableColumnFlags_None);

            for (auto &str : eumetsat_list)
            {
                ImGui::TableNextColumn();
                ImGui::Text("%s", str.timestamp.c_str());

                ImGui::TableNextColumn();
                if (ImGui::Button(std::string("Load##archiveloadertablebutton_" + str.timestamp).c_str()))
                {
                    std::string resp;
                    if (perform_http_request(str.href, resp, "") != 1)
                    {
                        nlohmann::json respj = nlohmann::json::parse(resp);
                        //                        printf("\n%s\n", respj.dump(4).c_str());

                        std::string nat_link, file_name;
                        if (eumetsat_products[eumetsat_selected_dataset].is_hdf)
                        {
                            auto &respj2 = respj["properties"]["links"]["data"];

                            for (auto &item : respj2.items())

                                if (item.value()["mediaType"].get<std::string>() == "application/zip")
                                {
                                    nat_link = item.value()["href"].get<std::string>();
                                    file_name = respj["id"].get<std::string>() + ".zip";
                                }
                        }
                        else
                        {
                            auto &respj2 = respj["properties"]["links"]["sip-entries"];

                            for (auto &item : respj2.items())

                                if (item.value()["mediaType"].get<std::string>() == "application/octet-stream")
                                {
                                    nat_link = item.value()["href"].get<std::string>();
                                    file_name = item.value()["title"].get<std::string>();
                                }
                        }

                        //logger->trace("\n%s\n", nat_link.c_str());

                        std::string download_path = products_download_and_process_directory + "/" + file_name;
                        std::string process_path = (download_location && output_selection.isValid() ?
                            output_selection.getPath() : products_download_and_process_directory) +
                            "/" + std::filesystem::path(file_name).stem().string();

                        auto func = [this, nat_link, download_path, process_path](int)
                        {
                            try
                            {
                                if (file_downloader.download_file(nat_link, download_path, "Authorization: Bearer " + getEumetSatToken()) != 1)
                                {
                                    processing::process("off2pro",
                                                        "file",
                                                        download_path,
                                                        process_path,
                                                        {});
                                }
                            }
                            catch (std::exception &e)
                            {
                                logger->error("Failed downloading file from EUMETSAT! %s", e.what());
                            }
                        };

                        ui_thread_pool.push(func);
                    }
                }
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();
        if (should_disable)
            style::endDisabled();
    }
}