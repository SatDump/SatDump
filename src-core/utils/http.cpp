#include "http.h"
#include "logger.h"
#include "satdump_vars.h"
#include <cstring>
#include <curl/curl.h>

#define CURL_TIMEOUT 5000

namespace satdump
{
    size_t curl_write_std_string(void *contents, size_t size, size_t nmemb, std::string *s)
    {
        size_t newLength = size * nmemb;
        try
        {
            s->append((char *)contents, newLength);
        }
        catch (std::bad_alloc &)
        {
            return 0;
        }
        return newLength;
    }

    int curl_float_progress_func(void *ptr, curl_off_t TotalToDownload, curl_off_t NowDownloaded, curl_off_t TotalToUpload, curl_off_t NowUploaded)
    {
        float *pptr = (float *)ptr;
        if (TotalToDownload != 0)
            *pptr = (float)NowDownloaded / (float)TotalToDownload;
        else if (TotalToUpload != 0)
            *pptr = (float)NowUploaded / (float)TotalToUpload;
        return 0;
    }

    int perform_http_request(std::string url_str, std::string &result, std::string added_header, float *progress)
    {
        CURL *curl;
        CURLcode res;
        bool ret = 1;
        char error_buffer[CURL_ERROR_SIZE] = {0};

        curl_global_init(CURL_GLOBAL_ALL);

        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string) "SatDump/v" + satdump::SATDUMP_VERSION).c_str());
            curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_std_string);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT);

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

            if (progress != nullptr)
            {
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progress);
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_float_progress_func);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
            }

            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                if (strlen(error_buffer))
                    logger->error("curl_easy_perform() failed: %s", error_buffer);
                else
                    logger->error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
                ret = 0;

            curl_easy_cleanup(curl);

            if (chunk != NULL)
                curl_slist_free_all(chunk);
        }
        curl_global_cleanup();
        return ret;
    }

    int perform_http_request_post(std::string url_str, std::string &result, std::string post_req, std::string added_header)
    {
        CURL *curl;
        CURLcode res;
        bool ret = 1;
        char error_buffer[CURL_ERROR_SIZE] = {0};

        curl_global_init(CURL_GLOBAL_ALL);

        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string) "SatDump/v" + satdump::SATDUMP_VERSION).c_str());
            curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_req.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_std_string);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT);

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

            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                if (strlen(error_buffer))
                    logger->error("curl_easy_perform() failed: %s", error_buffer);
                else
                    logger->error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
                ret = 0;

            curl_easy_cleanup(curl);

            if (chunk != NULL)
                curl_slist_free_all(chunk);
        }
        curl_global_cleanup();
        return ret;
    }
} // namespace satdump