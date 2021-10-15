#include "tle.h"
#include "resources.h"
#include "logger.h"
#include <map>
#include <fstream>
#include <filesystem>

//#ifndef __ANDROID__
//#include <nng/nng.h>
//#include <nng/supplemental/http/http.h>
//#endif

namespace tle
{
    std::map<int, TLE> tle_map;

    void loadTLEs()
    {
        std::string path = resources::getResourcePath("tle");

        if (std::filesystem::exists(path))
        {
            logger->info("Loading TLEs from " + path);

            std::filesystem::recursive_directory_iterator tleIterator(path);
            std::error_code iteratorError;
            while (tleIterator != std::filesystem::recursive_directory_iterator())
            {
                if (!std::filesystem::is_directory(tleIterator->path()))
                {
                    if (tleIterator->path().filename().string().find(".tle") != std::string::npos)
                    {
                        logger->trace("Found TLE file " + tleIterator->path().string());

                        // Parse
                        std::ifstream tle_file(tleIterator->path().string());

                        std::string line;
                        int line_count = 0;
                        int norad = 0;
                        std::string name, tle1, tle2;
                        while (getline(tle_file, line))
                        {
                            if (line_count % 3 == 0)
                            {
                                name = line;
                                name.erase(std::find_if(name.rbegin(), name.rend(), [](char &c)
                                                        { return c != ' '; })
                                               .base(),
                                           name.end()); // Remove useless spaces
                            }
                            else if (line_count % 3 == 1)
                            {
                                tle1 = line;
                            }
                            else if (line_count % 3 == 2)
                            {
                                tle2 = line;

                                std::string noradstr = tle2.substr(2, tle2.substr(2, tle2.size() - 1).find(' '));
                                norad = std::stoi(noradstr);

                                tle_map.emplace(std::pair<int, TLE>(norad, {norad, name, tle1, tle2}));
                            }

                            line_count++;
                        }
                    }
                }

                tleIterator.increment(iteratorError);
                if (iteratorError)
                    logger->critical(iteratorError.message());
            }
        }
        else
        {
            logger->warn("TLEs not found! Some things may not work.");
        }
    }

    TLE getTLEfromNORAD(int norad)
    {
        if (tle_map.count(norad) > 0)
        {
            return tle_map[norad];
        }
        else
        {
            logger->error("TLE from NORAD " + std::to_string(norad) + " not found!");
            return {norad, "", "", ""};
        }
    }

    /*
// Need to figure out why libnng won't run on Android...
#ifndef __ANDROID__
    int http_get(std::string url_str, std::string &result)
    {
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
            logger->error("HTTP Server Responded: %d %s", nng_http_res_get_status(res), nng_http_res_get_reason(res));
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

        return 0;
    }
#endif


    // Very temporary system
    std::vector<int> satellite_norads = {
        29499, // MetOp-A
        38771, // MetOp-B
        43689, // MetOp-C

        25338, // NOAA-15
        28654, // NOAA-18
        33591, // NOAA-19

        32958, // FengYun-3A
        37214, // FengYun-3B
        39260, // FengYun-3C
        43010, // FengYun-3D
        49008, // FengYun-3E

        35865, // METEOR-M 1
        40069, // METEOR-M 2
        44387, // METEOR-M 2-2

        25994, // Terra
        27424, // Aqua
        28376, // Aura

        37849, // Suomi NPP
        43013, // JPSS-1

        41240, // Jason-3
    };

    std::string tle_fetch_url = "http://www.celestrak.com/satcat/tle.php?CATNR=";

    void updateTLEs()
    {
#ifndef __ANDROID__
        std::string tle_path = resources::getResourcePath("tle");

        logger->info("Updating TLEs...");
        for (int norad : satellite_norads)
        {
            std::string url_str = tle_fetch_url + std::to_string(norad);
            logger->debug("Fetching from " + url_str);

            std::string output;
            if (!http_get(url_str, output))
            {
                std::string filename = tle_path + "/" + std::to_string(norad) + ".tle";
                logger->trace(filename);
                std::ofstream(filename).write((char *)output.c_str(), output.length());
            }
            else
            {
                logger->error("Could not fetch TLE!");
            }
        }
#else
        logger->critical("TLE Updates not yet supported on Android due to libnng!");
#endif
    }
    */
}