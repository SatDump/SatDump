#include "elektro.h"
#include "logger.h"
#include <filesystem>
#include "common/utils.h"

namespace
{
    std::vector<std::string> exec(const char *command)
    {
        FILE *fp;
        char *result = NULL;
        size_t len = 0;

        fflush(NULL);
        fp = popen(command, "r");
        if (fp == NULL)
        {
            printf("Cannot execute command:\n%s\n", command);
            return std::vector<std::string>();
        }

        std::vector<std::string> out;

        while (getline(&result, &len, fp) != -1)
        {
            std::string l(result);
            l = l.substr(0, l.size() - 1);
            out.push_back(l);
        }

        free(result);
        fflush(fp);
        if (pclose(fp) != 0)
        {
            perror("Cannot close stream.\n");
        }
        return out;
    }
}

std::vector<ELEKTROImage> get_list_of_current_day_elektro(time_t current_moscow_time, int satnum)
{
    std::vector<ELEKTROImage> outt;

    std::tm *timeReadable = gmtime(&current_moscow_time);
    // return std::to_string(timeReadable->tm_year + 1900) + "/" +
    //        (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
    //        (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + " " +
    //        (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
    //        (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
    //        (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

    std::string month;
    if (timeReadable->tm_mon + 1 == 1)
        month = "January";
    else if (timeReadable->tm_mon + 1 == 2)
        month = "February";
    else if (timeReadable->tm_mon + 1 == 3)
        month = "March";
    else if (timeReadable->tm_mon + 1 == 4)
        month = "April";
    else if (timeReadable->tm_mon + 1 == 5)
        month = "May";
    else if (timeReadable->tm_mon + 1 == 6)
        month = "June";
    else if (timeReadable->tm_mon + 1 == 7)
        month = "July";
    else if (timeReadable->tm_mon + 1 == 8)
        month = "August";
    else if (timeReadable->tm_mon + 1 == 9)
        month = "September";
    else if (timeReadable->tm_mon + 1 == 10)
        month = "October";
    else if (timeReadable->tm_mon + 1 == 11)
        month = "November";
    else if (timeReadable->tm_mon + 1 == 12)
        month = "December";

    std::string curr_link =
        (std::string) "ftp://electro:electro@ntsomz.gptl.ru:2121/" +
        "ELECTRO_L_" + std::to_string(satnum) + "/" +
        std::to_string(timeReadable->tm_year + 1900) +
        "/" + month + "/" +
        (timeReadable->tm_mday < 10 ? "0" : "") + std::to_string(timeReadable->tm_mday);

    std::string command = "ncftpls \"" + curr_link + "/*\"";

    logger->critical(command);
    auto output = exec(command.c_str());

    for (auto &out : output)
    {
        command = "ncftpls \"" + curr_link + "/" + out + "/*.zip\"";
        auto output2 = exec(command.c_str());
        for (auto &out2 : output2)
        {
            std::string full_path = curr_link + "/" + out + out2;

            int hours = std::stoi(out.substr(0, 2));
            int minutes = std::stoi(out.substr(2, 2));

            std::tm tm_changing = *timeReadable;
            tm_changing.tm_hour = hours;
            tm_changing.tm_min = minutes;
            tm_changing.tm_sec = 0;

            time_t imgtime = mktime(&tm_changing) - 2 * 3600; // 3 * 3600;

            logger->info("%s %s", full_path.c_str(), timestamp_to_string(imgtime).c_str());

            outt.push_back({full_path, imgtime});
        }
    }

    return outt;
}

std::vector<ccsds::CCSDSPacket> encode_elektro_dump(std::string path, time_t timestamp, int apid)
{
    time_t timestamp_curr = timestamp;

    std::vector<ccsds::CCSDSPacket> encoded_pkts;

    std::filesystem::recursive_directory_iterator imgIterator(path);
    std::error_code iteratorError;
    while (imgIterator != std::filesystem::recursive_directory_iterator())
    {
        if (!std::filesystem::is_directory(imgIterator->path()))
        {
            if (imgIterator->path().filename().string().find(".jpg") != std::string::npos)
            {
                std::string path = imgIterator->path().string();
                std::string name = imgIterator->path().stem().string();
                logger->debug(name);

                int year, month, day;
                int hour, minutes;
                int channel;
                if (sscanf(name.c_str(), "%2d%2d%2d_%2d%2d_%d",
                           &year, &month, &day, &hour, &minutes, &channel) == 6)
                {
                    image::Image<uint8_t> source_image;
                    source_image.load_img(path);

                    // if (channel == 1 || channel == 2 || channel == 3)
                    //     source_image.resize_bilinear(4096, 4096);
                    // else
                    source_image.resize_bilinear(2048, 2048);

                    auto pkts = encode_image_chunked(source_image, timestamp_curr,
                                                     apid, channel, 65e3); // channel < 4 ? 7e3 : 1e3);
                    encoded_pkts.insert(encoded_pkts.end(), pkts.begin(), pkts.end());
                }
            }
        }

        imgIterator.increment(iteratorError);
        if (iteratorError)
            logger->critical(iteratorError.message());
    }

    return encoded_pkts;
}
