#include "utils.h"
#include <cmath>
#include <sstream>
#include "resources.h"
#include <locale>
#include <codecvt>

void signed_soft_to_unsigned(int8_t *in, uint8_t *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++)
    {
        out[i] = in[i] + 127;

        if (out[i] == 128) // 128 is for erased syms
            out[i] = 127;
    }
}

std::vector<std::string> splitString(std::string input, char del)
{
    std::stringstream stcStream(input);
    std::string seg;
    std::vector<std::string> segs;

    while (std::getline(stcStream, seg, del))
        segs.push_back(seg);

    return segs;
}

// Return filesize
uint64_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    uint64_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

bool isStringPresent(std::string searched, std::string keyword)
{
    std::transform(searched.begin(), searched.end(), searched.begin(), tolower);
    std::transform(keyword.begin(), keyword.end(), keyword.begin(), tolower);

    auto found_it = searched.find(keyword, 0);
    return found_it != std::string::npos;
}

#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#include "logger.h"
#include "satdump_vars.h"

#if defined(NNG_OPT_TLS_CONFIG)
#include <nng/supplemental/tls/tls.h>
#endif

int perform_http_request(std::string url_str, std::string &result)
{
    nng_http_client *client;
    nng_url *url;
    nng_aio *aio;
    nng_http_req *req;
    nng_http_res *res;
    int rv;

    int return_val = 0;

    if (((rv = nng_url_parse(&url, url_str.c_str())) != 0) ||
        ((rv = nng_http_client_alloc(&client, url)) != 0))
    {
        if (rv == NNG_ENOTSUP)
            logger->trace("Protocol not supported!");
        return 1;
    }

// HTTPS
#if defined(NNG_OPT_TLS_CONFIG)
    nng_tls_config *tls_config;
    nng_tls_config_alloc(&tls_config, NNG_TLS_MODE_CLIENT);
    nng_tls_config_auth_mode(tls_config, NNG_TLS_AUTH_MODE_NONE);
    nng_http_client_set_tls(client, tls_config);
#endif

    if (((rv = nng_http_req_alloc(&req, url)) != 0) ||
        ((rv = nng_http_res_alloc(&res)) != 0) ||
        ((rv = nng_aio_alloc(&aio, NULL, NULL)) != 0))
        return 1;

    nng_aio_set_timeout(aio, 30000);

    nng_http_req_add_header(req, "User-Agent", std::string("SatDump/v" + (std::string)SATDUMP_VERSION).c_str());

    // Start operation
    nng_http_client_transact(client, req, res, aio);

    if (nng_http_res_get_status(res) != NNG_HTTP_STATUS_OK)
    {
        logger->trace("HTTP Server Responded: %d %s", nng_http_res_get_status(res), nng_http_res_get_reason(res));
        return 1;
    }

    // Wait for it to complete.
    nng_aio_wait(aio);

    if ((rv = nng_aio_result(aio)) != 0)
    {
        logger->trace("HTTP Request Error!");
        return_val = 1;
    }

    // Load result
    char *data_ptr;
    size_t data_sz = 0;
    nng_http_res_get_data(res, (void **)&data_ptr, &data_sz);

    result = std::string(data_ptr, &data_ptr[data_sz]);

    // Free everything
    nng_http_client_free(client);
    nng_aio_free(aio);
    nng_http_res_free(res);
    nng_http_req_free(req);

#if defined(NNG_OPT_TLS_CONFIG)
    nng_tls_config_free(tls_config);
#endif

    return return_val;
}

std::string timestamp_to_string(double timestamp)
{
    if (timestamp < 0)
        timestamp = 0;
    time_t tttime = timestamp;
    std::tm *timeReadable = gmtime(&tttime);
    return std::to_string(timeReadable->tm_year + 1900) + "/" +
           (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
           (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + " " +
           (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
           (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
           (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
}

double get_median(std::vector<double> values)
{
    if (values.size() == 0)
        return 0;
    std::sort(values.begin(), values.end());
    size_t middle = values.size() / 2;
    return values[middle];
}

std::string loadFileToString(std::string path)
{
    std::ifstream f(path);
    std::string str = std::string(std::istreambuf_iterator<char>{f}, {});
    f.close();
    return str;
}

std::string ws2s(const std::wstring &wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.to_bytes(wstr);
}

std::wstring s2ws(const std::string &str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.from_bytes(str);
}