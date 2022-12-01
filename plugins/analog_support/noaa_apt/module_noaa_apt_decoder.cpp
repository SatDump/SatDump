#include "module_noaa_apt_decoder.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "products/dataset.h"

#include "common/dsp/firdes.h"

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

        std::ifstream data_in;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        logger->info("Using input wav " + d_input_file);

        // Init stream source
        std::shared_ptr<dsp::stream<float>> input_stream = std::make_shared<dsp::stream<float>>();
        auto feeder_func = [this, &input_stream, &data_in]()
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

                volk_16i_s32f_convert_32f_u((float *)input_stream->writeBuf, (const int16_t *)s16_buf, 65535, buffer_size);
                input_stream->swap(buffer_size);

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
                v = (v / 0.5f) * 255.0f;

                if (image_i < APT_IMG_WIDTH * APT_IMG_OVERS * APT_MAX_LINES)
                    wip_apt_image[image_i++] = wip_apt_image.clamp(v);
            }

            ctm->output_stream->flush();

            int line_cnt = image_i / (APT_IMG_WIDTH * APT_IMG_OVERS);

            if (textureID != 0 && line_cnt > last_line_cnt)
            {
                auto preview = wip_apt_image.resize_to(wip_apt_image.width() / 20, wip_apt_image.height() / 20);
                uchar_to_rgba(preview.data(), textureBuffer, preview.size());
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

        // Line mumbers
        int line_cnt = image_i / (APT_IMG_WIDTH * APT_IMG_OVERS);
        logger->info("Got {:d} lines...", line_cnt);
        // wip_apt_image.crop(0, 0, APT_IMG_WIDTH, line_cnt*2);

        // WB
        logger->info("White balance...");
        wip_apt_image.white_balance();

        // Synchronize
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

        image::Image<uint8_t> wip_apt_image_sync(APT_IMG_WIDTH, line_cnt, 1);

        logger->info("Synchronize...");
        for (int line = 0; line < line_cnt - 1; line++)
        {
            int best_cor = 40 * 255 * APT_IMG_OVERS;
            int best_pos = 0;
            for (int pos = 0; pos < APT_IMG_WIDTH * APT_IMG_OVERS; pos++)
            {
                int cor = 0;
                for (int i = 0; i < 40 * APT_IMG_OVERS; i++)
                {
                    cor += abs(int(wip_apt_image[line * APT_IMG_WIDTH * APT_IMG_OVERS + pos + i] - final_sync_a[i]));
                }

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

        std::string main_dir = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        logger->info("Saving...");
        wip_apt_image_sync.save_png(main_dir + "/raw.png");

        // Products ARE not yet being processed properly. Need to parse the wedges!
        satdump::ProductDataSet dataset;
        dataset.satellite_name = "NOAA";
        dataset.timestamp = time(0);

        // AVHRR
        {
            logger->info("----------- AVHRR");
            logger->info("Lines : " + std::to_string(line_cnt));

            satdump::ImageProducts avhrr_products;
            avhrr_products.instrument_name = "avhrr_3";
            // avhrr_products.has_timestamps = true;
            // avhrr_products.set_tle(satellite_tle);
            avhrr_products.bit_depth = 8;
            // avhrr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            // avhrr_products.set_timestamps(avhrr_reader.timestamps);
            // avhrr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_avhrr.json")));

            // std::string names[6] = {"1", "2", "3a", "3b", "4", "5"};
            // for (int i = 0; i < 6; i++)
            //     avhrr_products.images.push_back({"AVHRR-" + names[i] + ".png", names[i], avhrr_reader.getChannel(i)});

            image::Image<uint8_t> ch1, ch2;
            ch1 = wip_apt_image_sync.crop_to(86, 86 + 909);
            ch2 = wip_apt_image_sync.crop_to(1126, 1126 + 909);

            avhrr_products.images.push_back({"APT-1.png", "1", ch1.to16bits()});
            avhrr_products.images.push_back({"APT-2.png", "2", ch2.to16bits()});

            avhrr_products.save(main_dir);
            dataset.products_list.push_back(".");
        }

        dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
    }

    void NOAAAPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA APT Decoder (WIP!)", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            if (textureID == 0)
            {
                textureID = makeImageTexture();
                textureBuffer = new uint32_t[((APT_IMG_WIDTH * APT_IMG_OVERS) / 20) * (APT_MAX_LINES / 20)];
            }

            if (has_to_update)
            {
                updateImageTexture(textureID, textureBuffer, wip_apt_image.width() / 20, wip_apt_image.height() / 20);
                has_to_update = false;
            }

            ImGui::Image((void *)(intptr_t)textureID, {200 * ui_scale, 200 * ui_scale});
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