#include "module_noaa_apt_decoder.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "products/dataset.h"

#include "common/dsp/firdes.h"
#include "resources.h"

#include "common/wav.h"

namespace noaa_apt
{
    NOAAAPTDecoderModule::NOAAAPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
        if (parameters.count("audio_samplerate") > 0)
            d_audio_samplerate = parameters["audio_samplerate"].get<long>();
        else
            throw std::runtime_error("Audio samplerate parameter must be present!");
    }

    NOAAAPTDecoderModule::~NOAAAPTDecoderModule()
    {
        if (textureID != 0)
        {
            delete[] textureBuffer;
            deleteImageTexture(textureID);
        }
    }

    void NOAAAPTDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        bool is_stereo = false;

        std::ifstream data_in;
        if (input_data_type == DATA_FILE)
        {
            wav::WavHeader hdr = wav::parseHeaderFromFileWav(d_input_file);
            if (!wav::isValidWav(hdr))
                std::runtime_error("File is not WAV!");
            if (hdr.bits_per_sample != 16)
                std::runtime_error("Only 16-bits WAV are supported!");
            d_audio_samplerate = hdr.samplerate;
            is_stereo = hdr.channel_cnt == 2;
            if (is_stereo)
                logger->info("File is stereo. Ignoring second channel!");

            wav::FileMetadata md = wav::tryParseFilenameMetadata(d_input_file, true);
            if (md.timestamp != 0 && (d_parameters.contains("start_timestamp") ? d_parameters["start_timestamp"] == -1 : 1))
                d_parameters["start_timestamp"] = md.timestamp;

            data_in = std::ifstream(d_input_file, std::ios::binary);
        }

        logger->info("Using input wav " + d_input_file);

        // Init stream source
        std::shared_ptr<dsp::stream<float>> input_stream = std::make_shared<dsp::stream<float>>();
        auto feeder_func = [this, &input_stream, &data_in, is_stereo]()
        {
            const int buffer_size = 1024;
            int16_t *s16_buf = new int16_t[buffer_size];
            time_t lastTime = 0;
            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)s16_buf, buffer_size * sizeof(int16_t));
                else
                    input_fifo->read((uint8_t *)s16_buf, buffer_size * sizeof(int16_t));

                if (is_stereo)
                {
                    for (int i = 0; i < buffer_size * 2; i++)
                        input_stream->writeBuf[i] = s16_buf[i * 2] / 65535.0;
                    input_stream->swap(buffer_size / 2);
                }
                else
                {
                    volk_16i_s32f_convert_32f_u((float *)input_stream->writeBuf, (const int16_t *)s16_buf, 65535, buffer_size);
                    input_stream->swap(buffer_size);
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }
            delete[] s16_buf;
        };

        // Init blocks
        rtc = std::make_shared<dsp::RealToComplexBlock>(input_stream);
        frs = std::make_shared<dsp::FreqShiftBlock>(rtc->output_stream, d_audio_samplerate, -2.4e3);
        int filter_srate = APT_IMG_WIDTH * 2 * APT_IMG_OVERS;
        rsp = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(frs->output_stream, filter_srate, d_audio_samplerate);
        int tofilter_freq = 1040;
        int gcd = std::gcd(filter_srate, tofilter_freq);
        filter_srate /= gcd;
        tofilter_freq /= gcd;
        lpf = std::make_shared<dsp::FIRBlock<complex_t>>(rsp->output_stream, dsp::firdes::low_pass(1, filter_srate, tofilter_freq, 0.5));
        ctm = std::make_shared<dsp::ComplexToMagBlock>(lpf->output_stream);

        // Start everything
        std::thread feeder_thread(feeder_func);
        rtc->start();
        frs->start();
        rsp->start();
        lpf->start();
        ctm->start();

        int image_i = 0;
        wip_apt_image.init(APT_IMG_WIDTH * APT_IMG_OVERS, APT_MAX_LINES, 1);

        int last_line_cnt = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            int nsamp = ctm->output_stream->read();

            for (int i = 0; i < nsamp; i++)
            {
                float v = ctm->output_stream->readBuf[i];
                v = (v / 0.5f) * 65535.0f;

                if (image_i < APT_IMG_WIDTH * APT_IMG_OVERS * APT_MAX_LINES)
                    wip_apt_image[image_i++] = wip_apt_image.clamp(v);
            }

            ctm->output_stream->flush();

            int line_cnt = image_i / (APT_IMG_WIDTH * APT_IMG_OVERS);

            if (textureID != 0 && line_cnt > last_line_cnt)
            {
                auto preview = wip_apt_image.resize_to(wip_apt_image.width() / 5, wip_apt_image.height() / 5);
                ushort_to_rgba(preview.data(), textureBuffer, preview.size());
                has_to_update = true;
                last_line_cnt = line_cnt;
            }
        }

        if (feeder_thread.joinable())
            feeder_thread.join();

        // Stop everything
        input_stream->clearReadStop();
        input_stream->clearWriteStop();
        rtc->stop();
        frs->stop();
        rsp->stop();
        lpf->stop();
        ctm->stop();

        if (input_data_type == DATA_FILE)
            data_in.close();

        apt_status = PROCESSING;

        // Line mumbers
        int line_cnt = image_i / (APT_IMG_WIDTH * APT_IMG_OVERS);
        logger->info("Got {:d} lines...", line_cnt);

        // WB
        logger->info("White balance...");
        wip_apt_image.white_balance();

        // Synchronize
        logger->info("Synchronize...");
        image::Image<uint16_t> wip_apt_image_sync = synchronize(line_cnt);

        // Save
        std::string main_dir = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        apt_status = SAVING;

        logger->info("Saving...");
        wip_apt_image_sync.save_png(main_dir + "/raw.png");

        // Products ARE not yet being processed properly. Need to parse the wedges!
        int norad = 0;
        std::string sat_name = "NOAA";
        std::optional<satdump::TLE> satellite_tle;

        if (d_parameters.contains("satellite_number"))
        {
            double number = 0;

            try
            {
                number = d_parameters["satellite_number"];
            }
            catch (std::exception &e)
            {
                number = std::stoi(d_parameters["satellite_number"].get<std::string>());
            }

            if (number == 15)
            { // N15
                norad = 25338;
                sat_name = "NOAA-15";
            }
            else if (number == 18)
            { // N18
                norad = 28654;
                sat_name = "NOAA-18";
            }
            else if (number == 19)
            { // N19
                norad = 33591;
                sat_name = "NOAA-19";
            }

            satellite_tle = satdump::general_tle_registry.get_from_norad(norad);

            // SATELLITE ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }
        }

        satdump::ProductDataSet dataset;
        dataset.satellite_name = sat_name;
        dataset.timestamp = time(0);

        // AVHRR
        {
            logger->info("----------- AVHRR");
            logger->info("Lines : " + std::to_string(line_cnt));

            satdump::ImageProducts avhrr_products;
            avhrr_products.instrument_name = "avhrr_apt";
            avhrr_products.bit_depth = 8;

            // std::string names[6] = {"1", "2", "3a", "3b", "4", "5"};
            // for (int i = 0; i < 6; i++)
            //     avhrr_products.images.push_back({"AVHRR-" + names[i] + ".png", names[i], avhrr_reader.getChannel(i)});

            if (d_parameters.contains("start_timestamp") && norad != 0)
            {
                double start_tt = d_parameters["start_timestamp"];

                if (start_tt != -1)
                {
                    std::vector<double> timestamps;

                    for (int i = 0; i < line_cnt; i++)
                        timestamps.push_back(start_tt + (double(i) * 0.5));

                    avhrr_products.has_timestamps = true;
                    avhrr_products.set_tle(satellite_tle);
                    avhrr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    avhrr_products.set_timestamps(timestamps);
                    if (norad == 25338)
                        avhrr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/noaa_15_avhrr_apt.json")));
                    else if (norad == 28654)
                        avhrr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/noaa_18_avhrr_apt.json")));
                    else if (norad == 33591)
                        avhrr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/noaa_19_avhrr_apt.json")));
                }
            }

            image::Image<uint16_t> cha, chb;
            cha = wip_apt_image_sync.crop_to(86, 86 + 909);
            chb = wip_apt_image_sync.crop_to(1126, 1126 + 909);

            avhrr_products.images.push_back({"APT-A.png", "a", cha});
            avhrr_products.images.push_back({"APT-B.png", "b", chb});

            avhrr_products.save(main_dir);
            dataset.products_list.push_back(".");
        }

        apt_status = DONE;

        dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
    }

    image::Image<uint16_t> NOAAAPTDecoderModule::synchronize(int line_cnt)
    {
        const int sync_a[] = {0, 0, 0, 0,
                              255, 255, 0, 0,
                              255, 255, 0, 0,
                              255, 255, 0, 0,
                              255, 255, 0, 0,
                              255, 255, 0, 0,
                              255, 255, 0, 0,
                              255, 255, 0, 0,
                              0, 0, 0, 0,
                              0, 0, 0, 0};

        std::vector<int> final_sync_a;
        for (int i = 0; i < 40; i++)
            for (int f = 0; f < APT_IMG_OVERS; f++)
                final_sync_a.push_back(sync_a[i]);

        image::Image<uint16_t> wip_apt_image_sync(APT_IMG_WIDTH, line_cnt, 1);

        for (int line = 0; line < line_cnt - 1; line++)
        {
            int best_cor = 40 * 255 * APT_IMG_OVERS;
            int best_pos = 0;
            for (int pos = 0; pos < APT_IMG_WIDTH * APT_IMG_OVERS; pos++)
            {
                int cor = 0;
                for (int i = 0; i < 40 * APT_IMG_OVERS; i++)
                    cor += abs(int((wip_apt_image[line * APT_IMG_WIDTH * APT_IMG_OVERS + pos + i] >> 8) - final_sync_a[i]));

                if (cor < best_cor)
                {
                    best_cor = cor;
                    best_pos = pos;
                }
            }

            // logger->critical("Line {:d} Pos {:d} Cor {:d}", line, best_pos, best_cor);
            for (int i = 0; i < APT_IMG_WIDTH; i++)
                wip_apt_image_sync[line * APT_IMG_WIDTH + i] = wip_apt_image[line * APT_IMG_WIDTH * APT_IMG_OVERS + best_pos + i * APT_IMG_OVERS];
        }

        return wip_apt_image_sync;
    }

    void NOAAAPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA APT Decoder (WIP!)", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            if (textureID == 0)
            {
                textureID = makeImageTexture();
                textureBuffer = new uint32_t[((APT_IMG_WIDTH * APT_IMG_OVERS) / 5) * (APT_MAX_LINES / 5)];
            }

            if (has_to_update)
            {
                updateImageTexture(textureID, textureBuffer, wip_apt_image.width() / 5, wip_apt_image.height() / 5);
                has_to_update = false;
            }

            ImGui::Image((void *)(intptr_t)textureID, {200 * ui_scale, 200 * ui_scale});
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
            drawStatus(apt_status);
        }
        ImGui::EndGroup();

        if (input_data_type == DATA_FILE)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NOAAAPTDecoderModule::getID()
    {
        return "noaa_apt_decoder";
    }

    std::vector<std::string> NOAAAPTDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> NOAAAPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NOAAAPTDecoderModule>(input_file, output_file_hint, parameters);
    }
}