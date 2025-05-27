#pragma once

/**
 * @file http.h
 */

#include <string>

namespace satdump
{
    /**
     * @brief cURL helper function, writing
     * the payload into a std::string
     *
     * TODOREWORK document
     */
    size_t curl_write_std_string(void *contents, size_t size, size_t nmemb, std::string *s);

    /**
     * @brief Perform a HTTP Request on the
     * provided URL and return the result as
     * a string.
     *
     * @param url_str URL to use
     * @param result HTTP response, as a string
     * @param added_header optional additional headers
     * @param progress optional progress float pointer
     */
    int perform_http_request(std::string url, std::string &result, std::string added_header = "", float *progress = nullptr);

    /**
     * @brief Perform a HTTP Request on the
     * provided URL and return the result as
     * a string, with POST data.
     *
     * @param url_str URL to use
     * @param result HTTP response, as a string
     * @param post_req POST payload
     * @param added_header optional additional headers
     */
    int perform_http_request_post(std::string url_str, std::string &result, std::string post_req, std::string added_header = "");
} // namespace satdump