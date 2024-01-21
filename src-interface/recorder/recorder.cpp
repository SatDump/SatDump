#include "recorder.h"
#include "core/config.h"
#include "common/utils.h"
#include "core/style.h"
#include "logger.h"
#include "imgui/imgui_stdlib.h"
#include "core/pipeline.h"
#include "common/widgets/stepped_slider.h"
#include "common/widgets/frequency_input.h"

#include "main_ui.h"

#include "imgui/imgui_internal.h"

#include "processing.h"

#include "resources.h"

namespace satdump
{
    RecorderApplication::RecorderApplication()
        : Application("recorder"), pipeline_selector(true)
    {
        dsp::registerAllSources();

        automated_live_output_dir = config::main_cfg["satdump_directories"]["live_processing_autogen"]["value"].get<bool>();
        processing_modules_floating_windows = config::main_cfg["user_interface"]["recorder_floating_windows"]["value"].get<bool>();

        sources = dsp::getAllAvailableSources();

        for (dsp::SourceDescriptor src : sources)
        {
            logger->debug("Device " + src.name);
            sdr_select_string += src.name + '\0';
        }

        // First, check to see if a source was passed via command line
        if (satdump::config::main_cfg.contains("cli") && satdump::config::main_cfg["cli"].contains("source"))
        {
            auto &cli_settings = satdump::config::main_cfg["cli"];
            std::string source = cli_settings["source"];
            for (int i = 0; i < (int)sources.size(); i++)
            {
                if (sources[i].source_type == source)
                {
                    if (cli_settings.contains("source_id") && cli_settings["source_id"].get<uint64_t>() != sources[i].unique_id)
                        continue;

                    try
                    {
                        source_ptr = dsp::getSourceFromDescriptor(sources[i]);
                        source_ptr->open();
                        sdr_select_id = i;
                        break;
                    }
                    catch (std::runtime_error &e)
                    {
                        logger->error(e.what());
                    }
                }
            }
        }

        // If not, or it failed to open, revert to last used SDR
        if (sdr_select_id == -1 && config::main_cfg["user"]["recorder_sdr_settings"].contains("last_used_sdr"))
        {
            std::string last_used_sdr = config::main_cfg["user"]["recorder_sdr_settings"]["last_used_sdr"].get<std::string>();
            for (int i = 0; i < (int)sources.size(); i++)
            {
                if (sources[i].name != last_used_sdr)
                    continue;
                try
                {
                    source_ptr = dsp::getSourceFromDescriptor(sources[i]);
                    source_ptr->open();
                    sdr_select_id = i;
                    break;
                }
                catch (std::runtime_error &e)
                {
                    logger->error(e.what());
                    break;
                }
            }
        }

        // If no source has been opened yet, open the first possible one
        for (int i = 0; sdr_select_id == -1 && i < (int)sources.size(); i++)
        {
            try
            {
                source_ptr = dsp::getSourceFromDescriptor(sources[i]);
                source_ptr->open();
                sdr_select_id = i;
            }
            catch (std::runtime_error &e)
            {
                logger->error(e.what());
            }
        }

        // Load waterfall palettes
        {
            waterfall_palettes_str = "";
            std::filesystem::recursive_directory_iterator paletteIterator(resources::getResourcePath("waterfall"));
            std::error_code iteratorError;
            while (paletteIterator != std::filesystem::recursive_directory_iterator())
            {
                if (!std::filesystem::is_directory(paletteIterator->path()))
                {
                    auto map = colormaps::loadMap(paletteIterator->path().string());
                    waterfall_palettes.push_back(map);
                    waterfall_palettes_str += map.name + " [" + map.author + "]" + '\0';
                }

                paletteIterator.increment(iteratorError);
                if (iteratorError)
                    logger->critical(iteratorError.message());
            }
        }

        set_frequency(frequency_hz);
        try_load_sdr_settings();

        splitter = std::make_shared<dsp::SplitterBlock>(source_ptr->output_stream);
        splitter->add_output("record");
        splitter->add_output("live");

        fft = std::make_shared<dsp::FFTPanBlock>(splitter->output_stream);
        fft->set_fft_settings(fft_size, get_samplerate(), fft_rate);
        fft->avg_num = 10;
        fft->start();

        file_sink = std::make_shared<dsp::FileSinkBlock>(splitter->get_output("record"));
        file_sink->start();

        fft_plot = std::make_shared<widgets::FFTPlot>(fft->output_stream->writeBuf, fft_size, -10, 20, 10);
        fft_plot->frequency = frequency_hz;
        waterfall_plot = std::make_shared<widgets::WaterfallPlot>(fft_sizes_lut[0], 500);

        fft->on_fft = [this](float *v)
        { waterfall_plot->push_fft(v); };

        // Load config
        if (config::main_cfg["user"].contains("recorder_state"))
            deserialize_config(config::main_cfg["user"]["recorder_state"]);

        set_output_sample_format();
        fft_plot->set_size(fft_size);
        waterfall_plot->set_size(fft_size);
        waterfall_plot->set_rate(fft_rate, waterfall_rate);

        // Attempt to apply provided CLI settings
        if (satdump::config::main_cfg.contains("cli"))
        {
            auto &cli_settings = satdump::config::main_cfg["cli"];
            if (source_ptr)
            {
                if (cli_settings.contains("samplerate"))
                    source_ptr->set_samplerate(cli_settings["samplerate"].get<uint64_t>());
                source_ptr->set_settings(cli_settings);
            }

            if (cli_settings.contains("start_recorder_device") && cli_settings["start_recorder_device"].get<bool>())
            {
                logger->warn("Recorder was asked to autostart!");
                start();
            }

            if (cli_settings.contains("engage_autotrack") && cli_settings["engage_autotrack"].get<bool>())
            {
                tracking_started_cli = true;
                try_init_tracking_widget();
            }
        }
    }

    RecorderApplication::~RecorderApplication()
    {
    retry_vfo:
        for (auto &vfo : vfo_list)
        {
            del_vfo(vfo.id);
            goto retry_vfo;
        }

        stop_processing();
        if (is_started)
            stop();
        source_ptr->close();
        splitter->input_stream = std::make_shared<dsp::stream<complex_t>>();
        splitter->stop();
        fft->stop();
        file_sink->stop();

        if (tracking_widget != nullptr)
            delete tracking_widget;
        if (constellation_debug != nullptr)
            delete constellation_debug;
    }

    void RecorderApplication::drawUI()
    {
        ImVec2 recorder_size = ImGui::GetContentRegionAvail();

        if (ImGui::BeginTable("##recorder_table", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_None, recorder_size.x * panel_ratio);
            ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_None, recorder_size.x * (1.0f - panel_ratio));
            ImGui::TableNextColumn();

            float left_width = ImGui::GetColumnWidth(0);
            float right_width = recorder_size.x - left_width;
            if (left_width != last_width && last_width != -1)
                panel_ratio = left_width / recorder_size.x;
            last_width = left_width;

            ImGui::BeginGroup();
            float wf_size_offset = 0;
            if (is_processing && !processing_modules_floating_windows)
                wf_size_offset = 250 * ui_scale;
            if (vfo_list.size() > 0)
                wf_size_offset = 270 * ui_scale;
            float wf_size = recorder_size.y - wf_size_offset; // + 13 * ui_scale;

            ImGui::BeginChild("RecorderChildPanel", {left_width, wf_size}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            {
                if (ImGui::CollapsingHeader("Device", tracking_started_cli ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Spacing();
                    if (is_started)
                        style::beginDisabled();
                    if (ImGui::Combo("##Source", &sdr_select_id, sdr_select_string.c_str()))
                    {
                        // Try to open a device, if it doesn't work, we re-open a device we can
                        try
                        {
                            source_ptr.reset();
                            source_ptr = getSourceFromDescriptor(sources[sdr_select_id]);
                            source_ptr->open();
                        }
                        catch (std::runtime_error &e)
                        {
                            logger->error("Could not open device! %s", e.what());

                            for (int i = 0; i < (int)sources.size(); i++)
                            {
                                try
                                {
                                    source_ptr.reset();
                                    source_ptr = dsp::getSourceFromDescriptor(sources[i]);
                                    source_ptr->open();
                                    sdr_select_id = i;
                                    break;
                                }
                                catch (std::runtime_error &e)
                                {
                                    logger->error(e.what());
                                }
                            }
                        }

                        set_frequency(frequency_hz);
                        try_load_sdr_settings();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8" \uead2 "))
                    {
                        sources = dsp::getAllAvailableSources();

                        sdr_select_string.clear();
                        for (dsp::SourceDescriptor src : sources)
                        {
                            logger->debug("Device " + src.name);
                            sdr_select_string += src.name + '\0';
                        }

                        while (sdr_select_id >= (int)sources.size())
                            sdr_select_id--;

                        source_ptr = getSourceFromDescriptor(sources[sdr_select_id]);
                        source_ptr->open();
                        set_frequency(frequency_hz);
                        try_load_sdr_settings();
                    }
                    /*
                    if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                            ImGui::TextUnformatted("Refresh source list");
                            ImGui::PopTextWrapPos();
                            ImGui::EndTooltip();
                        }
                    */

                    ImGui::InputInt("Decimation##recorderdecimation", &current_decimation);
                    if (current_decimation < 1)
                        current_decimation = 1;

                    bool pushed_color_xconv = xconverter_frequency != 0;
                    if (pushed_color_xconv)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 255, 0, 255));
                    if (ImGui::InputDouble("MHz (LO offset)##downupconverter", &xconverter_frequency))
                        set_frequency(frequency_hz);
                    if (pushed_color_xconv)
                        ImGui::PopStyleColor();

                    if (is_started)
                        style::endDisabled();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (widgets::FrequencyInput("Hz##mainfreq", &frequency_hz))
                        set_frequency(frequency_hz);

                    ImGui::Spacing();
                    source_ptr->drawControlUI();

                    if (!is_started)
                    {
                        if (ImGui::Button("Start"))
                            start();
                    }
                    else
                    {
                        if (ImGui::Button("Stop"))
                            stop();
                    }

                    sdr_error.draw();
                }

                if (ImGui::CollapsingHeader("FFT", tracking_started_cli ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::Combo("FFT Size", &selected_fft_size, "131072\0"
                                                                     "65536\0"
                                                                     "16384\0"
                                                                     "8192\0"
                                                                     "4096\0"
                                                                     "2048\0"
                                                                     "1024\0"))
                    {
                        fft_size = fft_sizes_lut[selected_fft_size];

                        fft->set_fft_settings(fft_size, get_samplerate(), fft_rate);
                        fft_plot->set_size(fft_size);
                        waterfall_plot->set_size(fft_size);

                        logger->info("Set FFT size to %d", fft_size);
                    }
                    if (ImGui::InputInt("FFT Rate", &fft_rate))
                    {
                        fft_size = fft_sizes_lut[selected_fft_size];

                        fft->set_fft_settings(fft_size, get_samplerate(), fft_rate);
                        waterfall_plot->set_rate(fft_rate, waterfall_rate);

                        logger->info("Set FFT rate to %d", fft_rate);
                    }
                    if (ImGui::InputInt("Waterfall Rate", &waterfall_rate))
                    {
                        waterfall_plot->set_rate(fft_rate, waterfall_rate);
                        logger->info("Set Waterfall rate to %d", waterfall_rate);
                    }
                    widgets::SteppedSliderFloat("FFT Max", &fft_plot->scale_max, -150, 150);
                    widgets::SteppedSliderFloat("FFT Min", &fft_plot->scale_min, -150, 150);
                    widgets::SteppedSliderFloat("Avg Num", &fft->avg_num, 1, 500, 1);
                    if (ImGui::Combo("Palette", &selected_waterfall_palette, waterfall_palettes_str.c_str()))
                        waterfall_plot->set_palette(waterfall_palettes[selected_waterfall_palette]);
                    ImGui::Checkbox("Show Waterfall", &show_waterfall);
                    ImGui::Checkbox("Frequency Scale", &fft_plot->enable_freq_scale);
                }

                if (fft_plot->scale_max < fft_plot->scale_min)
                {
                    fft_plot->scale_min = waterfall_plot->scale_min;
                    fft_plot->scale_max = waterfall_plot->scale_max;
                }
                else if (fft_plot->scale_min > fft_plot->scale_max)
                {
                    fft_plot->scale_min = waterfall_plot->scale_min;
                    fft_plot->scale_max = waterfall_plot->scale_max;
                }
                else
                {
                    waterfall_plot->scale_min = fft_plot->scale_min;
                    waterfall_plot->scale_max = fft_plot->scale_max;
                }

                if (ImGui::CollapsingHeader("Processing"))
                {
                    // Settings & Selection menu
                    if (is_processing)
                        style::beginDisabled();
                    pipeline_selector.renderSelectionBox(ImGui::GetContentRegionAvail().x);
                    if (!automated_live_output_dir)
                        pipeline_selector.drawMainparamsLive();
                    pipeline_selector.renderParamTable();
                    if (is_processing)
                        style::endDisabled();

                    // Preset Menu
                    Pipeline selected_pipeline = pipelines[pipeline_selector.pipeline_id];
                    if (selected_pipeline.preset.frequencies.size() > 0)
                    {
                        if (ImGui::BeginCombo("Freq###presetscombo", selected_pipeline.preset.frequencies[pipeline_preset_id].second == frequency_hz ? selected_pipeline.preset.frequencies[pipeline_preset_id].first.c_str() : ""))
                        {
                            for (int n = 0; n < (int)selected_pipeline.preset.frequencies.size(); n++)
                            {
                                const bool is_selected = (pipeline_preset_id == n);
                                if (ImGui::Selectable(selected_pipeline.preset.frequencies[n].first.c_str(), is_selected))
                                {
                                    pipeline_preset_id = n;

                                    if (selected_pipeline.preset.frequencies[pipeline_preset_id].second != 0)
                                    {
                                        frequency_hz = selected_pipeline.preset.frequencies[pipeline_preset_id].second;
                                        set_frequency(frequency_hz);
                                    }
                                }

                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }

                    if (!is_started)
                        style::beginDisabled();

                    if (!is_processing)
                    {
                        if (ImGui::Button("Start###startprocessing"))
                            start_processing();
                    }
                    else
                    {
                        if (ImGui::Button("Stop##stopprocessing"))
                            stop_processing();
                    }

                    error.draw();

                    if (!is_started)
                        style::endDisabled();
                }

                if (ImGui::CollapsingHeader("Recording"))
                {
                    if (is_recording)
                        style::beginDisabled();
                    if (ImGui::Combo("Format", &select_sample_format, "f32\0"
                                                                      "s16\0"
                                                                      "s8\0"
                                                                      "wav16\0"
#ifdef BUILD_ZIQ
                                                                      "ziq s8\0"
                                                                      "ziq s16\0"
                                                                      "ziq f32\0"
#endif
#ifdef BUILD_ZIQ2
                                                                      "ziq2 s8 (WIP)\0"
                                                                      "ziq2 s16 (WIP)\0"
#endif
                                     ))
                        set_output_sample_format();

                    if (is_recording)
                        style::endDisabled();

                    if (file_sink->get_written() < 1e9)
                        ImGui::Text("Size : %.2f MB", file_sink->get_written() / 1e6);
                    else
                        ImGui::Text("Size : %.2f GB", file_sink->get_written() / 1e9);

#ifdef BUILD_ZIQ
                    if (select_sample_format == 4 || select_sample_format == 5 || select_sample_format == 6)
                    {
                        if (file_sink->get_written_raw() < 1e9)
                            ImGui::Text("Size (raw) : %.2f MB", file_sink->get_written_raw() / 1e6);
                        else
                            ImGui::Text("Size (raw) : %.2f GB", file_sink->get_written_raw() / 1e9);
                    }
#endif

                    ImGui::Text("File : %s", recorder_filename.c_str());

                    ImGui::Spacing();

                    if (!is_recording)
                        ImGui::TextColored(ImColor(255, 0, 0), "IDLE");
                    else
                        ImGui::TextColored(ImColor(0, 255, 0), "RECORDING");

                    ImGui::Spacing();

                    if (!is_recording)
                    {
                        if (ImGui::Button("Start###startrecording"))
                            start_recording();
                    }
                    else
                    {
                        if (ImGui::Button("Stop##stoprecording"))
                            stop_recording();
                    }
                }

                show_tracking = ImGui::CollapsingHeader("Tracking", tracking_started_cli ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
                if (show_tracking)
                {
                    try_init_tracking_widget();
                    tracking_widget->render();
                }

                if (ImGui::CollapsingHeader("VFOs"))
                {
                    vfos_mtx.lock();

                    if (ImGui::BeginTable("##eosinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Name");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("Frequency");
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("Pipeline");

                        for (auto &vfo : vfo_list)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%s", vfo.name.c_str());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", format_notated(vfo.freq, "Hz").c_str());
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", pipelines[vfo.pipeline_id].readable_name.c_str());
                        }

                        ImGui::EndTable();
                    }

                    vfos_mtx.unlock();

#if 0
                    if (ImGui::Button("Add Test"))
                    {
                        int idp = 0;
                        for (int n = 0; n < (int)pipelines.size(); n++)
                        {
                            if ("meteor_m2-x_lrpt" == pipelines[n].name)
                                idp = n;
                        }

                        add_vfo("meteor_test", "Meteor Test", 137.9e6, idp, {});
                    }

                    if (ImGui::Button("Del Test"))
                    {
                        del_vfo("meteor_test");
                    }
#endif
                }

                if (ImGui::CollapsingHeader("Debug"))
                {
                    if (constellation_debug == nullptr)
                        constellation_debug = new widgets::ConstellationViewer();
                    if (is_started)
                        constellation_debug->pushComplex(source_ptr->output_stream->readBuf, 256);
                    constellation_debug->draw();
                }
            }
            ImGui::EndChild();
            ImGui::EndGroup();

            ImGui::TableNextColumn();
            ImGui::BeginGroup();
            ImGui::BeginChild("RecorderFFT", {right_width, wf_size}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            {
                float fft_height = wf_size * (show_waterfall ? waterfall_ratio : 1.0);
                float wf_height = wf_size * (1 - waterfall_ratio) + 15 * ui_scale;
                float wfft_widht = right_width - 9 * ui_scale;
                bool t = true;
#ifdef __ANDROID__
                int offset = 8;
#else
                int offset = 30;
#endif
                ImGui::SetNextWindowSizeConstraints(ImVec2((right_width + offset * ui_scale), 50), ImVec2((right_width + offset * ui_scale), wf_size));
                ImGui::SetNextWindowSize(ImVec2((right_width + offset * ui_scale), show_waterfall ? waterfall_ratio * wf_size : wf_size));
                ImGui::SetNextWindowPos(ImVec2(left_width, 25 * ui_scale));
                if (ImGui::Begin("#fft", &t, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9 * ui_scale);
                    fft_plot->draw({float(wfft_widht), fft_height});
                    if (show_waterfall && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                        waterfall_ratio = ImGui::GetWindowHeight() / wf_size;
                    if (ImGui::IsWindowHovered())
                    {
                        ImVec2 mouse_pos = ImGui::GetMousePos();
                        float ratio = (mouse_pos.x - left_width - 16 * ui_scale) / (right_width - 9 * ui_scale) - 0.5;
                        ImGui::SetTooltip("%s", ((ratio >= 0 ? "" : "- ") + format_notated(abs(ratio) * get_samplerate(), "Hz\n") +
                                                 format_notated(source_ptr->get_frequency() + ratio * get_samplerate(), "Hz"))
                                                    .c_str());
                    }
                    ImGui::EndChild();
                }
                if (show_waterfall)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15 * ui_scale);
                    waterfall_plot->draw({wfft_widht, wf_height}, is_started);
                }
            }
            ImGui::EndChild();
            ImGui::EndGroup();
            ImGui::EndTable();
        }

        if (vfo_list.size() > 0)
        {
            if (ImGui::BeginTabBar("VFO TabBar", ImGuiTabBarFlags_None))
            {
                if (is_processing)
                {
                    if (ImGui::BeginTabItem("Offline processing"))
                    {
                        float y_pos = ImGui::GetCursorPosY() + 35 * ui_scale;
                        float live_width = recorder_size.x + 16 * ui_scale;
                        float live_height = 250 * ui_scale;
                        float winwidth = live_pipeline->modules.size() > 0 ? live_width / live_pipeline->modules.size() : live_width;
                        float currentPos = 0;
                        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));
                        for (std::shared_ptr<ProcessingModule> &module : live_pipeline->modules)
                        {
                            ImGui::SetNextWindowPos({currentPos, y_pos});
                            ImGui::SetNextWindowSize({(float)winwidth, (float)live_height});
                            module->drawUI(false);
                            currentPos += winwidth;

                            // if (ImGui::GetCurrentContext()->last_window != NULL)
                            //     currentPos += ImGui::GetCurrentContext()->last_window->Size.x;
                            //  logger->info(ImGui::GetCurrentContext()->last_window->Name);
                        }
                        ImGui::PopStyleColor();
                        ImGui::EndTabItem();
                    }
                }

                vfos_mtx.lock();
                for (auto &vfo : vfo_list)
                {
                    if (ImGui::BeginTabItem(vfo.name.c_str()))
                    {
                        float y_pos = ImGui::GetCursorPosY() + 35 * ui_scale;
                        float live_width = recorder_size.x + 16 * ui_scale;
                        float live_height = 250 * ui_scale;
                        float winwidth = vfo.live_pipeline->modules.size() > 0 ? live_width / vfo.live_pipeline->modules.size() : live_width;
                        float currentPos = 0;
                        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));
                        for (std::shared_ptr<ProcessingModule> &module : vfo.live_pipeline->modules)
                        {
                            ImGui::SetNextWindowPos({currentPos, y_pos});
                            ImGui::SetNextWindowSize({(float)winwidth, (float)live_height});
                            module->drawUI(false);
                            currentPos += winwidth;

                            // if (ImGui::GetCurrentContext()->last_window != NULL)
                            //     currentPos += ImGui::GetCurrentContext()->last_window->Size.x;
                            //  logger->info(ImGui::GetCurrentContext()->last_window->Name);
                        }
                        ImGui::PopStyleColor();
                        ImGui::EndTabItem();
                    }
                }
                vfos_mtx.unlock();

                ImGui::EndTabBar();
            }
        }
        else if (is_processing)
        {
            if (processing_modules_floating_windows)
            {
                for (std::shared_ptr<ProcessingModule> &module : live_pipeline->modules)
                    module->drawUI(true);
            }
            else
            {
                float y_pos = ImGui::GetCursorPosY() + 35 * ui_scale;
                float live_width = recorder_size.x + 16 * ui_scale;
                float live_height = 250 * ui_scale;
                float winwidth = live_pipeline->modules.size() > 0 ? live_width / live_pipeline->modules.size() : live_width;
                float currentPos = 0;
                ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));
                for (std::shared_ptr<ProcessingModule> &module : live_pipeline->modules)
                {
                    ImGui::SetNextWindowPos({currentPos, y_pos});
                    ImGui::SetNextWindowSize({(float)winwidth, (float)live_height});
                    module->drawUI(false);
                    currentPos += winwidth;

                    // if (ImGui::GetCurrentContext()->last_window != NULL)
                    //     currentPos += ImGui::GetCurrentContext()->last_window->Size.x;
                    //  logger->info(ImGui::GetCurrentContext()->last_window->Name);
                }
                ImGui::PopStyleColor();
            }
        }

        // Keyboard shortcuts
        {
            // FFT
            if (ImGui::IsKeyDown(ImGuiKey_ModShift) && ImGui::IsKeyDown(ImGuiKey_X))
            {
                if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
                    fft_plot->scale_max -= 0.5;
                else if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
                    fft_plot->scale_max += 0.5;
                else if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
                    fft_plot->scale_min -= 0.5;
                else if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
                    fft_plot->scale_min += 0.5;
#if 0
                else if (ImGui::IsKeyDown(ImGuiKey_PageUp))
                    fft->avg_rate += 0.001;
                else if (ImGui::IsKeyDown(ImGuiKey_PageDown))
                    fft->avg_rate -= 0.001;

                if (fft->avg_rate > 0.99)
                    fft->avg_rate = 0.99;
                else if (fft->avg_rate < 0.01)
                    fft->avg_rate = 0.01;
#endif
            }
        }
    }
};
