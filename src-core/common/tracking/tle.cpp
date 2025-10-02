#include <optional>
#include <string>
#include <vector>
#define SATDUMP_DLL_EXPORT 1
#include "common/utils.h"
#include "core/config.h"
#include "core/plugin.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "satdump_vars.h"
#include "tle.h"
#include "utils/http.h"
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace satdump
{
    int parseTLEStream(std::istream &inputStream, std::vector<TLE> &new_registry)
    {
        std::string this_line;
        std::deque<std::string> tle_lines;
        int total_lines = 0;
        while (std::getline(inputStream, this_line))
        {
            this_line.erase(0, this_line.find_first_not_of(" \t\n\r"));
            this_line.erase(this_line.find_last_not_of(" \t\n\r") + 1);
            if (this_line != "")
                tle_lines.push_back(this_line);

            if (tle_lines.size() == 3)
            {
                bool checksum_good = true;
                for (int i = 1; i < 3; i++)
                {
                    if (!isdigit(tle_lines[i].back()) || !isdigit(tle_lines[i].front()) || tle_lines[i].front() - '0' != i)
                    {
                        checksum_good = false;
                        break;
                    }

                    int checksum = tle_lines[i].back() - '0';
                    int actualsum = 0;
                    for (int j = 0; j < (int)tle_lines[i].size() - 1; j++)
                    {
                        if (tle_lines[i][j] == '-')
                            actualsum++;
                        else if (isdigit(tle_lines[i][j]))
                            actualsum += tle_lines[i][j] - '0';
                    }

                    checksum_good = checksum == actualsum % 10;
                    if (!checksum_good)
                        break;
                }
                if (!checksum_good)
                {
                    tle_lines.pop_front();
                    continue;
                }

                int norad;
                try
                {
                    std::string noradstr = tle_lines[2].substr(2, tle_lines[2].substr(2, tle_lines[2].size() - 1).find(' '));
                    norad = std::stoi(noradstr);
                }
                catch (std::exception &)
                {
                    tle_lines.pop_front();
                    continue;
                }

                new_registry.push_back({norad, tle_lines[0], tle_lines[1], tle_lines[2]});
                tle_lines.clear();
                total_lines++;
            }
        }

        // Sort and remove duplicates
        sort(new_registry.begin(), new_registry.end(), [](TLE &a, TLE &b) { return a.name < b.name; });
        new_registry.erase(std::unique(new_registry.begin(), new_registry.end(), [](TLE &a, TLE &b) { return a.norad == b.norad; }), new_registry.end());

        return total_lines;
    }

    std::vector<TLE> tryFetchTLEsFromFileURL(std::string url_str)
    {
        bool success = true;
        std::vector<TLE> new_registry;

        logger->info(url_str);
        std::string result;
        int http_res = 1, trials = 0;
        while (http_res == 1 && trials < 10)
        {
            if ((http_res = satdump::perform_http_request(url_str, result)) != 1)
            {
                std::istringstream tle_stream(result);
                success = parseTLEStream(tle_stream, new_registry) > 0;
            }
            else
                success = false;
            trials++;
            if (!success)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                logger->info("Failed getting TLEs. Retrying...");
            }
        }

        if (!success)
            logger->warn("Failed to get TLE for %s", url_str.c_str());

        return new_registry;
    }

    std::vector<TLE> tryFetchSingleTLEwithNorad(int norad)
    {
        bool success = true;
        std::vector<TLE> new_registry;

        std::string url_str = satdump_cfg.main_cfg["tle_settings"]["url_template"].get<std::string>();
        while (url_str.find("%NORAD%") != std::string::npos)
            url_str.replace(url_str.find("%NORAD%"), 7, std::to_string(norad));

        logger->info(url_str);
        std::string result;
        int http_res = 1, trials = 0;
        while (http_res == 1 && trials < 10)
        {
            if ((http_res = perform_http_request(url_str, result)) != 1)
            {
                std::istringstream tle_stream(result);
                success = parseTLEStream(tle_stream, new_registry) > 0;
            }
            else
                success = false;
            trials++;
            if (!success)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                logger->info("Failed getting TLEs. Retrying...");
            }
        }

        if (!success)
            logger->error("Error updating TLE for %d. Ignoring!", norad);

        return new_registry;
    }

    std::optional<TLE> get_from_spacetrack_time(int norad, time_t timestamp)
    {
        time_t last_update = getValueOrDefault<time_t>(satdump_cfg.main_cfg["user"]["tles_last_updated"], 0);
        std::string sc_login = satdump_cfg.getValueFromSatDumpGeneral<std::string>("tle_space_track_login");
        std::string sc_passw = satdump_cfg.getValueFromSatDumpGeneral<std::string>("tle_space_track_password");

        // Use local time if Space Track credentials are not entered, or time is close to TLE catalog time
        if (sc_login == "" || sc_passw == "" || sc_login == "yourloginemail" || sc_passw == "yourpassword" || fabs((double)last_update - (double)timestamp) < 4 * 24 * 3600)
            return std::optional<TLE>();

        // Otherwise, request on Space-Track's archive
        logger->trace("Pulling historical TLE from Space Track...");
        CURL *curl;
        CURLcode res;
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();

        if (!curl)
        {
            logger->warn("Failed to pull historical TLE due to internal curl failure! Using current TLE");
            curl_global_cleanup();
            return std::optional<TLE>();
        }

        // Get the cookie
        std::string post_fields = "identity=" + sc_login + "&password=" + sc_passw;
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string) "SatDump/v" + SATDUMP_VERSION).c_str());
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.space-track.org/ajaxauth/login");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

#ifdef CURLSSLOPT_NATIVE_CA
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            logger->warn("Failed to authenticate to Space Track! Using current TLE");
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return std::optional<TLE>();
        }

        // Get the actual TLE
        std::string timestamp_day, timestamp_daytime;
        {
            if (timestamp < 0)
                timestamp = 0;
            time_t tttime = timestamp;
            std::tm *timeReadable = gmtime(&tttime);
            timestamp_day = std::to_string(timeReadable->tm_year + 1900) + "-" +
                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0");
            timestamp_daytime = (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "%3A" +
                                (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "%3A" +
                                (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
        }

        std::string final_url = "https://www.space-track.org/basicspacedata/query/class/gp_history/NORAD_CAT_ID/" + std::to_string(norad) + "/EPOCH/%3C" + timestamp_day + "T" + timestamp_daytime +
                                "/orderby/EPOCH%20desc/limit/1/emptyresult/show";
        std::string result;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl, CURLOPT_POST, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, satdump::curl_write_std_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_URL, final_url.c_str());

        logger->trace("Request URL : %s", final_url.c_str());

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            logger->warn("Failed to download TLE from Space Track! Using built-in TLE");
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return std::optional<TLE>();
        }

        // Log out and clean up
        std::string logout_result;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &logout_result);
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.space-track.org/ajaxauth/logout");
        curl_easy_perform(curl); // We do not care about the result
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        // Parse the downloaded TLE
        try
        {
            bool parsed = true;
            nlohmann::json res;
            try
            {
                res = nlohmann::json::parse(result)[0];
            }
            catch (std::exception &)
            {
                parsed = false;
            }
            if (!parsed || !res.contains("TLE_LINE0") || !res.contains("TLE_LINE1") || !res.contains("TLE_LINE2"))
            {
                logger->warn("Error pulling TLE from Space-Track! Returned data: %s", result.c_str());
                return std::optional<TLE>();
            }

            TLE tle;
            tle.norad = norad;
            tle.name = res["TLE_LINE0"].get<std::string>().substr(2, res["TLE_LINE0"].get<std::string>().size());
            tle.line1 = res["TLE_LINE1"].get<std::string>();
            tle.line2 = res["TLE_LINE2"].get<std::string>();
            tle.time = timestamp; // TODOREWORK actual time!!!!
            return tle;
        }
        catch (std::exception &e)
        {
            logger->error("Could not get TLE from Space-Track : %s", e.what());
            return std::optional<TLE>();
        }

        return std::optional<TLE>();
    }

    std::vector<TLE> get_from_spacetrack_latest_list(std::vector<int> norad)
    {
        time_t last_update = getValueOrDefault<time_t>(satdump_cfg.main_cfg["user"]["tles_last_updated"], 0);
        std::string sc_login = satdump_cfg.getValueFromSatDumpGeneral<std::string>("tle_space_track_login");
        std::string sc_passw = satdump_cfg.getValueFromSatDumpGeneral<std::string>("tle_space_track_password");

        // Otherwise, request on Space-Track's archive
        logger->warn("Pulling current TLEs from Space Track... (ONLY COVERS OBJECTS CURRENTLY IN DATABASE!)");
        CURL *curl;
        CURLcode res;
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();

        if (!curl)
        {
            logger->warn("Failed to pull current TLEs due to internal curl failure! Using current TLE");
            curl_global_cleanup();
            return std::vector<TLE>();
        }

        // Get the cookie
        std::string post_fields = "identity=" + sc_login + "&password=" + sc_passw;
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string) "SatDump/v" + SATDUMP_VERSION).c_str());
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.space-track.org/ajaxauth/login");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

#ifdef CURLSSLOPT_NATIVE_CA
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            logger->warn("Failed to authenticate to Space Track! Using current TLE");
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return std::vector<TLE>();
        }

        std::string final_url = "https://www.space-track.org/basicspacedata/query/class/gp/NORAD_CAT_ID/";
        for (int i = 0; i < norad.size() - 1; i++)
            final_url += std::to_string(norad[i]) + "%2C";
        final_url += std::to_string(norad[norad.size() - 1]);
        final_url += "/orderby/NORAD_CAT_ID%20desc/emptyresult/show";

        std::string result;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl, CURLOPT_POST, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, satdump::curl_write_std_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_URL, final_url.c_str());

        logger->trace("Request URL : %s", final_url.c_str());

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            logger->warn("Failed to download TLE from Space Track! Using built-in TLE");
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return std::vector<TLE>();
        }

        // Log out and clean up
        std::string logout_result;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &logout_result);
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.space-track.org/ajaxauth/logout");
        curl_easy_perform(curl); // We do not care about the result
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        // Parse the downloaded TLE
        try
        {
            bool parsed = true;
            nlohmann::json res;
            try
            {
                res = nlohmann::json::parse(result);
            }
            catch (std::exception &)
            {
                parsed = false;
            }

            std::vector<TLE> tles;
            for (int g = 0; g < res.size(); g++)
            {
                auto &f = res[g];

                TLE tle;
                tle.norad = std::stoi(f["NORAD_CAT_ID"].get<std::string>());
                tle.name = f["TLE_LINE0"].get<std::string>().substr(2, f["TLE_LINE0"].get<std::string>().size());
                tle.line1 = f["TLE_LINE1"].get<std::string>();
                tle.line2 = f["TLE_LINE2"].get<std::string>();
                tles.push_back(tle);
            }
            return tles;
        }
        catch (std::exception &e)
        {
            logger->error("Could not get current TLEs from Space-Track : %s", e.what());
            return std::vector<TLE>();
        }

        return std::vector<TLE>();
    }

    std::optional<TLE> get_from_norad_in_vec(std::vector<TLE> vec, int norad)
    {
        std::vector<TLE>::iterator it = std::find_if(vec.begin(), vec.end(), [&norad](const TLE &e) { return e.norad == norad; });

        if (it != vec.end())
            return std::optional<TLE>(*it);
        else
            return std::optional<TLE>();
    }
} // namespace satdump
