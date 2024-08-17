#include "module_orbcomm_stx_auto_demod.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include "common/dsp_source_sink/format_notated.h"
#include "common/repack.h"

inline double calcFreq(int f, bool small = true)
{
    if (small)
    {
        if (f <= 0x40)
            f = 0x1 << 8 | f;
        else if (f >= 0x50)
            f = 0x0 << 8 | f;
    }
    return 137.0 + double(f) * 0.0025;
}

inline int fcs(uint8_t *data, int len)
{
    int i;
    unsigned char c0 = 0;
    unsigned char c1 = 0;
    for (i = 0; i < len; i++)
    {
        c0 = c0 + *(data + i);
        c1 = c1 + c0;
    }
    return ((long)(c0 + c1)); /* returns zero if buffer is error-free*/
}

namespace orbcomm
{
    OrbcommSTXAutoDemodModule::OrbcommSTXAutoDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : BaseDemodModule(input_file, output_file_hint, parameters), d_center_frequency(parameters["frequency"])
    {
        name = "Orbcomm STX Auto Demodulator";
        show_freq = false;

        MIN_SPS = 1;
        MAX_SPS = 1;
    }

    void OrbcommSTXAutoDemodModule::init()
    {
        BaseDemodModule::initb(false);

        splitter = std::make_shared<dsp::SplitterBlock>(agc->output_stream);
        splitter->set_main_enabled(false);

        frm_callback = [this](uint8_t *frms, int cnt)
        {
            freqs_to_push_mtx.lock();
            for (int fi = 0; fi < cnt; fi++)
            {
                uint8_t *frm = &frms[fi * 600];

                if (output_data_type == DATA_FILE)
                    data_out.write((char *)frm, 600);
                else
                    output_fifo->write((uint8_t *)frm, 600);

                for (int i = 0; i < 50; i++)
                {
                    if (frm[i * 12] == 0x65)
                    {
                        if (fcs(&frm[i * 12], 24) == 0)
                        {
                            double freq = calcFreq(frm[i * 12 + 5]);
                            logger->trace("Synchronization, Freq %f Mhz", freq);

                            freqs_to_push.push_back(freq * 1e6);
                        }
                    }
                    else if (frm[i * 12] == 0x1C)
                    {
                        if (fcs(&frm[i * 12], 12) == 0)
                        {
                            int pos = frm[i * 12 + 1] & 0xF;

                            std::reverse(&frm[i * 12 + 2], &frm[i * 12 + 10]);
                            shift_array_left(&frm[i * 12 + 2], 8, 4, &frm[i * 12 + 2]);
                            uint16_t values[5];
                            repackBytesTo12bits(&frm[i * 12 + 2], 8, values);

                            std::string frequencies = "";
                            for (int i = 0; i < 5; i++)
                            {
                                if (values[i] != 0)
                                {
                                    double freq = calcFreq(values[i], false);
                                    frequencies += std::to_string(freq) + " Mhz, ";

                                    freqs_to_push.push_back(freq * 1e6);
                                }
                            }
                            logger->trace("Channels %d : %s", pos, frequencies.c_str());
                        }
                    }
                }
            }
            freqs_to_push_mtx.unlock();
        };

        add_stx_link(137.2875e6);
        add_stx_link(137.3125e6);
        add_stx_link(137.25e6);
        add_stx_link(137.46e6);
        add_stx_link(137.7375e6);
        add_stx_link(137.8e6);
        add_stx_link(137.6625e6);
    }

    OrbcommSTXAutoDemodModule::~OrbcommSTXAutoDemodModule()
    {
    }

    void OrbcommSTXAutoDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
        {
            filesize = nfile_source.filesize(); // file_source->getFilesize();
            throw satdump_exception("The Orbcomm Auto STX Demodulator is live-only!");
        }
        else
            filesize = 0;

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".frm");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".frm");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        splitter->start();

        while (demod_should_run())
        {
            freqs_to_push_mtx.lock();
            if (freqs_to_push.size() > 0)
            {
                freqs_to_push_mtx.unlock();
                add_stx_link(freqs_to_push[0]);
                freqs_to_push_mtx.lock();

                freqs_to_push.erase(freqs_to_push.begin());
            }
            freqs_to_push_mtx.unlock();

            stx_link_lst_mtx.lock();
            // Update module stats
            for (auto &freq : stx_link_lst)
            {
                module_stats[freq.first]["snr"] = freq.second.demod->getSNR();
                module_stats[freq.first]["deframer_state"] = freq.second.demod->getDefState();
            }

            for (auto &freq : stx_link_lst)
            {
                if (time(0) - freq.second.last_ping > 3600 * 24)
                {
                    stx_link_lst_mtx.unlock();
                    del_stx_link(freq.first);
                    stx_link_lst_mtx.lock();
                    break;
                }
            }
            stx_link_lst_mtx.unlock();

            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (input_data_type == DATA_FILE)
                progress = nfile_source.position(); // file_source->getPosition();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void OrbcommSTXAutoDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        splitter->stop();
        logger->trace("Splitter stopped");

        for (auto &freq : stx_link_lst)
            freq.second.demod->stop();
        logger->trace("Demodulators stopped");

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void OrbcommSTXAutoDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            stx_link_lst_mtx.lock();
            if (ImGui::BeginTable("##orbcommsatellitesdemodtable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Frequency");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Freq (Raw)");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Offset");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("SNR");
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("State");

                for (auto &freq : stx_link_lst)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(style::theme.green, "%s", format_notated(freq.first, "Hz").c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%f", freq.first);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%f", d_center_frequency - freq.first);
                    ImGui::TableSetColumnIndex(3);
                    if (freq.second.demod->getSNR() > 10)
                        ImGui::TextColored(style::theme.green, "%f", freq.second.demod->getSNR());
                    else if (freq.second.demod->getSNR() > 2)
                        ImGui::TextColored(style::theme.orange, "%f", freq.second.demod->getSNR());
                    else
                        ImGui::TextColored(style::theme.red, "%f", freq.second.demod->getSNR());
                    ImGui::TableSetColumnIndex(4);
                    if (freq.second.demod->getDefState() == 8)
                        ImGui::TextColored(style::theme.green, "SYNCED");
                    else if (freq.second.demod->getDefState() == 6)
                        ImGui::TextColored(style::theme.orange, "SYNCING");
                    else
                        ImGui::TextColored(style::theme.red, "NOSYNC");
                }

                ImGui::EndTable();
            }
            stx_link_lst_mtx.unlock();
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        drawStopButton();

        ImGui::End();

        drawFFT();
    }

    std::string OrbcommSTXAutoDemodModule::getID()
    {
        return "orbcomm_stx_auto_demod";
    }

    std::vector<std::string> OrbcommSTXAutoDemodModule::getParameters()
    {
        std::vector<std::string> params;
        return params;
    }

    std::shared_ptr<ProcessingModule> OrbcommSTXAutoDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<OrbcommSTXAutoDemodModule>(input_file, output_file_hint, parameters);
    }
}