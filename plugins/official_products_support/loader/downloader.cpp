#include "downloader.h"
#include "imgui/imgui.h"
#include "satdump_vars.h"
#include "logger.h"
#include "core/style.h"
#include "common/dsp_source_sink/format_notated.h"

namespace widgets
{
    FileDownloaderWidget::FileDownloaderWidget()
    {
    }

    FileDownloaderWidget::~FileDownloaderWidget()
    {
    }

    bool FileDownloaderWidget::is_busy()
    {
        return is_downloading;
    }

    void FileDownloaderWidget::render()
    {
        ImGui::Text("Downloading : %s", file_downloading.c_str());
        ImGui::Text("%s / %s", format_notated(curSize, "B", 2).c_str(), format_notated(downloadSize, "B", 2).c_str());

        ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Abort").x - ImGui::GetStyle().ItemSpacing.x * 2.0f, 0));
        ImGui::SameLine();

        if (is_downloading)
            ImGui::PushStyleColor(ImGuiCol_Button, style::theme.red.Value);
        else
            style::beginDisabled();
        if (ImGui::Button("Abort"))
            should_abort = true;
        if (is_downloading)
            ImGui::PopStyleColor();
        else
            style::endDisabled();
    }

    int FileDownloaderWidget::download_file(std::string url_str, std::string output_file, std::string added_header)
    {
        is_downloading = true;
        file_downloading = output_file;

        CURL* curl;
        CURLcode res;
        bool ret = 1;
        char error_buffer[CURL_ERROR_SIZE] = { 0 };

        curl_global_init(CURL_GLOBAL_ALL);

        std::ofstream output_filestream(output_file, std::ios::binary);

        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, std::string((std::string)"SatDump/v" + satdump::SATDUMP_VERSION).c_str());
            curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_std_ofstream);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_filestream);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 100);

#ifdef CURLSSLOPT_NATIVE_CA
            curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

            struct curl_slist* chunk = NULL;
            if (added_header != "")
            {
                /* Remove a header curl would otherwise add by itself */
                chunk = curl_slist_append(chunk, added_header.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            }

            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_float_progress_file_func);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);

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

        output_filestream.close();
        is_downloading = false;
        file_downloading = "IDLE";
        curSize = 0;
        downloadSize = 0;

        return ret;
    }
}