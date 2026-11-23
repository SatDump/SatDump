#pragma once

/*
 * Fetching and checking the DCS platform tables (NOAA's PDT, and HADS).
 *
 * Kept on its own, with no SatDump dependencies, so that it can be compiled and tested alone.
 *
 * - Downloads follow redirects, because NOAA now serves the PDT through one, but only ever
 *   over https, at most five times, with size and time limits.
 * - A download is only accepted if it actually looks like the table that was asked for. A web
 *   page, an empty body or an error document is never saved over a good file.
 */

#include <curl/curl.h>
#include <cstring>
#include <string>

namespace goes
{
    namespace hrit
    {
        namespace dcs_tables
        {
            const size_t MAX_BYTES = 128 * 1024 * 1024; // the PDT is about 10 MB, HADS about 23 MB
            const size_t MIN_RECORDS = 1000;            // either table has tens of thousands

            inline size_t write_capped(void *contents, size_t size, size_t nmemb, void *userp)
            {
                std::string *out = (std::string *)userp;
                size_t bytes = size * nmemb;
                if (out->size() + bytes > MAX_BYTES)
                    return 0; // stops the transfer
                out->append((char *)contents, bytes);
                return bytes;
            }

            // Fetch url into result. Returns an empty string on success, otherwise the reason.
            inline std::string download(const std::string &url, const std::string &user_agent, std::string &result)
            {
                result.clear();
                if (url.rfind("https://", 0) != 0)
                    return "not an https address";

                curl_global_init(CURL_GLOBAL_ALL);
                CURL *curl = curl_easy_init();
                if (!curl)
                    return "could not start curl";

                char error_buffer[CURL_ERROR_SIZE] = {0};
                curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_capped);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

                // Follow redirects, but never off https, and not for ever
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
#if LIBCURL_VERSION_NUM >= 0x075500
                curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
                curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
                curl_easy_setopt(curl, CURLOPT_PROTOCOLS, (long)CURLPROTO_HTTPS);
                curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, (long)CURLPROTO_HTTPS);
#endif
                curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);       // a 404 page is not a table
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
                curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 500L); // give up on a transfer that stalls...
                curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);   // ...for a minute
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1800L);
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);          // this runs on a worker thread
#ifdef CURLSSLOPT_NATIVE_CA
                curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

                CURLcode res = curl_easy_perform(curl);
                long status = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
                curl_easy_cleanup(curl);

                if (res != CURLE_OK)
                {
                    result.clear();
                    return strlen(error_buffer) ? std::string(error_buffer) : std::string(curl_easy_strerror(res));
                }
                if (status != 200)
                {
                    result.clear();
                    return "HTTP status " + std::to_string(status);
                }
                return "";
            }

            inline bool is_hex(const std::string &text, size_t pos, size_t count)
            {
                if (text.size() < pos + count)
                    return false;
                for (size_t i = pos; i < pos + count; i++)
                {
                    char c = text[i];
                    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                        return false;
                }
                return true;
            }

            // Call fn(line) for every line of data
            template <typename F>
            inline void each_line(const std::string &data, F fn)
            {
                size_t start = 0;
                while (start < data.size())
                {
                    size_t end = data.find('\n', start);
                    if (end == std::string::npos)
                        end = data.size();
                    size_t len = end - start;
                    if (len > 0 && data[start + len - 1] == '\r')
                        len--;
                    fn(data.substr(start, len));
                    start = end + 1;
                }
            }

            // How many lines look like PDT records: fixed width, the platform address in columns 6 to 13
            inline size_t pdt_records(const std::string &data)
            {
                size_t found = 0;
                each_line(data, [&found](const std::string &line) { if (line.size() >= 94 && is_hex(line, 6, 8)) found++; });
                return found;
            }

            // How many lines look like HADS records: the address, then at least ten fields split by '|'
            inline size_t hads_records(const std::string &data)
            {
                size_t found = 0;
                each_line(data, [&found](const std::string &line) {
                    if (!is_hex(line, 0, 8) || line.size() < 9 || line[8] != '|')
                        return;
                    size_t bars = 0;
                    for (char c : line)
                        if (c == '|')
                            bars++;
                    if (bars >= 10)
                        found++;
                });
                return found;
            }

            // The PDT is fixed width in characters, but UTF-8: an accented letter is several bytes and
            // would push every later column along. Give back one byte per character.
            inline std::string fixed_columns(const std::string &line)
            {
                std::string out;
                out.reserve(line.size());
                for (unsigned char c : line)
                {
                    if ((c & 0xC0) == 0x80)
                        continue; // the rest of a character already counted
                    out.push_back(c < 0x80 ? (char)c : '?');
                }
                return out;
            }

            // The position columns hold either degrees, minutes and seconds ("DDMMSS") or, since the
            // table moved to DADDS, ten-thousandths of a degree (" 409581" is 40.9581). Minutes or
            // seconds above 59 anywhere in the file can only be the second kind.
            inline bool pdt_is_decimal(const std::string &data, size_t lat_offset)
            {
                bool decimal = false;
                each_line(data, [&](const std::string &raw) {
                    if (decimal)
                        return;
                    std::string line = fixed_columns(raw);
                    if (line.size() < lat_offset + 7 || !is_hex(line, 6, 8))
                        return;
                    for (size_t at : {lat_offset + 3, lat_offset + 5})
                        if (line[at] >= '6' && line[at] <= '9')
                            decimal = true;
                });
                return decimal;
            }

            // Latitude and longitude of one PDT line. Throws like std::stof on a line that has none.
            inline void pdt_position(const std::string &line, size_t lat_offset, size_t lon_offset, bool decimal, float &lat, float &lon)
            {
                if (decimal)
                {
                    lat = std::stof(line.substr(lat_offset, 7)) / 10000.0f;
                    lon = std::stof(line.substr(lon_offset, 8)) / 10000.0f;
                    return;
                }
                lat = std::stof(line.substr(lat_offset + 1, 2));
                lat += std::stof(line.substr(lat_offset + 3, 2)) / 60.0f;
                lat += std::stof(line.substr(lat_offset + 5, 2)) / 3600.0f;
                lon = std::stof(line.substr(lon_offset + 1, 3));
                lon += std::stof(line.substr(lon_offset + 4, 2)) / 60.0f;
                lon += std::stof(line.substr(lon_offset + 6, 2)) / 3600.0f;
                if (line[lat_offset] == '-')
                    lat *= -1;
                if (line[lon_offset] == '-')
                    lon *= -1;
            }
        }
    }
}
