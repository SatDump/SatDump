#pragma once

#include <fstream>
#include <string>
#include <curl/curl.h>

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

        static int curl_float_progress_file_func(void *ptr, curl_off_t TotalToDownload, curl_off_t NowDownloaded, curl_off_t, curl_off_t)
        {
            FileDownloaderWidget *pptr = (FileDownloaderWidget *)ptr;
            if (TotalToDownload != 0)
                pptr->progress = (float)NowDownloaded / (float)TotalToDownload;
            pptr->curSize = NowDownloaded;
            pptr->downloadSize = TotalToDownload;

            if (pptr->should_abort)
            {
                pptr->should_abort = false;
                return 1;
            }
            else
                return 0;
        }

    private:
        bool is_downloading = false;
        bool should_abort = false;
        float progress = 0;
        std::string file_downloading = "IDLE";
        double curSize = 0, downloadSize = 0;

    public:
        FileDownloaderWidget();
        ~FileDownloaderWidget();
        bool is_busy();
        void render();
        int download_file(std::string url_str, std::string output_file, std::string added_header);
    };
}