#include "utils.h"
#include <cmath>
#include <sstream>
#include "resources.h"
#include <locale>
#include <codecvt>

#include "core/config.h"
#include "common/dsp_source_sink/format_notated.h"

#include <curl/curl.h>

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

#include "logger.h"
#include "satdump_vars.h"

size_t curl_write_std_string(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size * nmemb;
    try
    {
        s->append((char *)contents, newLength);
    }
    catch (std::bad_alloc &e)
    {
        return 0;
    }
    return newLength;
}

int perform_http_request(std::string url_str, std::string &result)
{
    CURL *curl;
    CURLcode res;
    bool ret = 1;

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string) "SatDump/v" + SATDUMP_VERSION).c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_std_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        ret = 0;
    }
    curl_global_cleanup();
    return ret;
}

int perform_http_request_post(std::string url_str, std::string &result, std::string post_req)
{
    CURL *curl;
    CURLcode res;
    bool ret = 1;

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string) "SatDump/v" + SATDUMP_VERSION).c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_req.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_std_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        ret = 0;
    }
    curl_global_cleanup();
    return ret;
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

std::string prepareAutomatedPipelineFolder(time_t timevalue, double frequency, std::string pipeline_name, std::string folder)
{
    std::tm *timeReadable = gmtime(&timevalue);
    if (folder == "")
    {
        folder = satdump::config::main_cfg["satdump_directories"]["live_processing_path"]["value"].get<std::string>();
#ifdef __ANDROID__
        if (folder == "./live_output")
            folder = "/storage/emulated/0/live_output";
#endif
    }
    std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));
    std::string output_dir = folder + "/" + timestamp + "_" + pipeline_name + "_" + format_notated(frequency, "Hz");
    std::filesystem::create_directories(output_dir);
    logger->info("Generated folder name : " + output_dir);
    return output_dir;
}

std::string prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency)
{
    const time_t timevalue = timeValue_precise;
    std::tm *timeReadable = gmtime(&timevalue);
    std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                            (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

    if (satdump::config::main_cfg["user_interface"]["recorder_baseband_filename_millis_precision"]["value"].get<bool>())
    {
        std::ostringstream ss;

        double ms_val = fmod(timeValue_precise, 1.0) * 1e3;
        ss << "-" << std::fixed << std::setprecision(0) << std::setw(3) << std::setfill('0') << ms_val;
        timestamp += ss.str();
    }

    return timestamp + "_" + std::to_string(samplerate) + "SPS_" + std::to_string(frequency) + "Hz";
}

void hsv_to_rgb(float h, float s, float v, uint8_t *rgb)
{
    float out_r, out_g, out_b;

    if (s == 0.0f)
    {
        // gray
        out_r = out_g = out_b = v;

        rgb[0] = out_r * 255;
        rgb[1] = out_g * 255;
        rgb[2] = out_b * 255;

        return;
    }

    h = fmod(h, 1.0f) / (60.0f / 360.0f);
    int i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0:
        out_r = v;
        out_g = t;
        out_b = p;
        break;
    case 1:
        out_r = q;
        out_g = v;
        out_b = p;
        break;
    case 2:
        out_r = p;
        out_g = v;
        out_b = t;
        break;
    case 3:
        out_r = p;
        out_g = q;
        out_b = v;
        break;
    case 4:
        out_r = t;
        out_g = p;
        out_b = v;
        break;
    case 5:
    default:
        out_r = v;
        out_g = p;
        out_b = q;
        break;
    }

    rgb[0] = out_r * 255;
    rgb[1] = out_g * 255;
    rgb[2] = out_b * 255;
}
