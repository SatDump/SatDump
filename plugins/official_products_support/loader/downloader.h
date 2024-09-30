#pragma once

#include "imgui/imgui.h"
#include <string>
#include <curl/curl.h>
#include <fstream>
#include "satdump_vars.h"
#include "logger.h"
#include "common/dsp_source_sink/format_notated.h"

namespace widgets
{
    class FileDownloaderWidget
    {
    private:
        static size_t curl_write_std_ofstream(void *contents, size_t size, size_t nmemb, std::ofstream *s)
        {
            size_t newLength = size * nmemb;
            try
            {
                s->write((char *)contents, newLength);
            }
            catch (std::bad_alloc &)
            {
                return 0;
            }
            return newLength;
        }

        static size_t curl_float_progress_file_func(void *ptr, double TotalToDownload, double NowDownloaded, double, double)
        {
            FileDownloaderWidget *pptr = (FileDownloaderWidget *)ptr;
            if (TotalToDownload != 0)
                pptr->progress = NowDownloaded / TotalToDownload;
            pptr->curSize = NowDownloaded;
            pptr->downloadSize = TotalToDownload;
            return 0;
        }

    private:
        bool is_downloading = false;
        float progress = 0;
        std::string file_downloading = "IDLE";
        double curSize = 0, downloadSize = 0;

    public:
        FileDownloaderWidget()
        {
        }

        ~FileDownloaderWidget()
        {
        }

        bool is_busy()
        {
            return is_downloading;
        }

        void render()
        {
            ImGui::Text("Downloading : %s", file_downloading.c_str());
            ImGui::Text("%s / %s", format_notated(curSize, "B", 2).c_str(), format_notated(downloadSize, "B", 2).c_str());
            ImGui::ProgressBar(progress);
        }

        int download_file(std::string url_str, std::string output_file, std::string added_header)
        {
            is_downloading = true;
            file_downloading = output_file;

            CURL *curl;
            CURLcode res;
            bool ret = 1;
            char error_buffer[CURL_ERROR_SIZE] = {0};

            curl_global_init(CURL_GLOBAL_ALL);

            std::ofstream output_filestream(output_file, std::ios::binary);

            curl = curl_easy_init();
            if (curl)
            {
                curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string) "SatDump/v" + SATDUMP_VERSION).c_str());
                curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_std_ofstream);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_filestream);
                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);

#ifdef CURLSSLOPT_NATIVE_CA
                curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

                struct curl_slist *chunk = NULL;
                if (added_header != "")
                {
                    /* Remove a header curl would otherwise add by itself */
                    chunk = curl_slist_append(chunk, added_header.c_str());
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
                }

                curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
                curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curl_float_progress_file_func);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);

                res = curl_easy_perform(curl);

                if (res != CURLE_OK)
                {
                    if (strlen(error_buffer))
                        logger->error("curl_easy_perform() failed: %s", error_buffer);
                    else
                        logger->error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
                }

                curl_easy_cleanup(curl);
                ret = 0;

                if (chunk != NULL)
                    curl_slist_free_all(chunk);
            }
            curl_global_cleanup();

            output_filestream.close();
            is_downloading = false;
            file_downloading = "IDLE";
            curSize = 0;
            downloadSize = 0;

            return ret;
        }
    };
}