#include "utils.h"
#include <cmath>
#include <sstream>

void char_array_to_uchar(int8_t *in, uint8_t *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++)
    {
        double r = in[i] * 127.0;
        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;
        out[i] = floor(r);
    }
}

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
size_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::size_t fileSize = file.tellg();
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

#ifndef __ANDROID__
#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#endif
#include "logger.h"

int perform_http_request(std::string url_str, std::string &result)
{
#ifndef __ANDROID__
    nng_http_client *client;
    nng_http_conn *conn;
    nng_url *url;
    nng_aio *aio;
    nng_http_req *req;
    nng_http_res *res;
    const char *hdr;
    int rv;
    int len;
    char *data;
    nng_iov iov;

    if (((rv = nng_url_parse(&url, url_str.c_str())) != 0) || ((rv = nng_http_client_alloc(&client, url)) != 0) ||
        ((rv = nng_http_req_alloc(&req, url)) != 0) || ((rv = nng_http_res_alloc(&res)) != 0) ||
        ((rv = nng_aio_alloc(&aio, NULL, NULL)) != 0))
        return 1;

    // Start connection process...
    nng_http_client_connect(client, aio);

    // Wait for it to finish.
    nng_aio_wait(aio);
    if ((rv = nng_aio_result(aio)) != 0)
        return 1;

    // Get the connection, at the 0th output.
    conn = (nng_http_conn *)nng_aio_get_output(aio, 0);

    nng_http_conn_write_req(conn, req, aio);
    nng_aio_wait(aio);

    if ((rv = nng_aio_result(aio)) != 0)
        return 1;

    // Read a response.
    nng_http_conn_read_res(conn, res, aio);
    nng_aio_wait(aio);

    if ((rv = nng_aio_result(aio)) != 0)
        return 1;

    if (nng_http_res_get_status(res) != NNG_HTTP_STATUS_OK)
    {
        logger->error("HTTP Server Responded: {:d} {:s}", nng_http_res_get_status(res), nng_http_res_get_reason(res));
        return 1;
    }

    // This only supports regular transfer encoding (no Chunked-Encoding,
    // and a Content-Length header is required.)
    if ((hdr = nng_http_res_get_header(res, "Content-Length")) == NULL)
    {
        logger->error("Missing Content-Length header.");
        return 1;
    }

    len = atoi(hdr);
    if (len == 0)
        return 1;

    // Allocate a buffer to receive the body data.
    data = (char *)malloc(len);

    // Set up a single iov to point to the buffer.
    iov.iov_len = len;
    iov.iov_buf = data;

    // Following never fails with fewer than 5 elements.
    nng_aio_set_iov(aio, 1, &iov);

    // Now attempt to receive the data.
    nng_http_conn_read_all(conn, aio);

    // Wait for it to complete.
    nng_aio_wait(aio);

    if ((rv = nng_aio_result(aio)) != 0)
    {
        // fatal(rv);
        logger->info("error");
    }

    result = std::string(data, &data[len]);

    nng_http_client_free(client);
    nng_aio_free(aio);
    nng_http_res_free(res);
    nng_http_req_free(req);
    free(data);
#endif

    return 0;
}

std::string timestamp_to_string(double timestamp)
{
    time_t tttime = timestamp;
    std::tm *timeReadable = gmtime(&tttime);
    return std::to_string(timeReadable->tm_year + 1900) + "/" +
           (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
           (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + " " +
           (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
           (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
           (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
}