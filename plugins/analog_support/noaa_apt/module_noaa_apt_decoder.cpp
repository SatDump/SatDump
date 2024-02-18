#include "module_noaa_apt_decoder.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "products/dataset.h"
#include "nlohmann/json_utils.h"

#include "common/dsp/filter/firdes.h"
#include "resources.h"

#include "common/wav.h"

#include "common/calibration.h"

#define MAX_WEDGE_DIFF_VALID 12000

namespace noaa_apt
{
    NOAAAPTDecoderModule::NOAAAPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
        if (parameters.count("audio_samplerate") > 0)
            d_audio_samplerate = parameters["audio_samplerate"].get<long>();
        else
            throw std::runtime_error("Audio samplerate parameter must be present!");

        if (parameters.count("autocrop_wedges") > 0)
            d_autocrop_wedges = parameters["autocrop_wedges"].get<bool>();
        if (parameters.count("save_unsynced") > 0)
            save_unsynced = parameters["save_unsynced"].get<bool>();
    }

    NOAAAPTDecoderModule::~NOAAAPTDecoderModule()
    {
        if (textureID != 0)
        {
            delete[] textureBuffer;
            // deleteImageTexture(textureID);
        }
    }

    template <typename T>
    inline void scale_val(T &valv, int &new_black, int &new_white)
    {
        float init_val = valv;
        init_val -= new_black;
        float vval = init_val / new_white;
        vval *= 65535;
        if (vval < 0)
            vval = 0;
        if (vval > 65535)
            vval = 65535;
        valv = vval;
    }

    void NOAAAPTDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        std::string main_dir = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        bool is_stereo = false;

        int autodetected_sat = -1;

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
            {
                d_parameters["start_timestamp"] = md.timestamp;
                logger->trace("Has timestamp %d", md.timestamp);
            }
            else
            {
                logger->warn("Couldn't automatically determine the timestamp, in case of unexpected results, please verify you have specified the correct timestamp manually");
            }

            if (md.frequency == 0 && d_parameters.contains("frequency"))
                md.frequency = d_parameters["frequency"].get<uint64_t>();

            logger->trace("Freq from meta : %llu", md.frequency);

            if (abs(md.frequency - 137.1e6) < 1e4)
            {
                autodetected_sat = 19;
                logger->info("Detected NOAA-19");
            }
            else if (abs(md.frequency - 137.9125e6) < 1e4)
            {
                autodetected_sat = 18;
                logger->info("Detected NOAA-18");
            }
            else if (abs(md.frequency - 137.62e6) < 1e4)
            {
                autodetected_sat = 15;
                logger->info("Detected NOAA-15");
            }
            else
            {
                logger->warn("Couldn't automatically determine the satellite, in case of unexpected results, please verify you have specified the correct satellite manually");
            }

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
                    for (int i = 0; i < buffer_size / 2; i++)
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
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
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
        int last_line_cnt = 0;
        std::vector<uint16_t> imagebuf;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            int nsamp = ctm->output_stream->read();

            for (int i = 0; i < nsamp; i++)
            {
                float v = ctm->output_stream->readBuf[i];
                v = (v / 0.5f) * 65535.0f;
                imagebuf.push_back(wip_apt_image.clamp(v));
                image_i++;
            }

            ctm->output_stream->flush();

            int line_cnt = image_i / (APT_IMG_WIDTH * APT_IMG_OVERS);
            if (textureID != 0 && line_cnt > last_line_cnt)
            {
                int x_scale = (APT_IMG_WIDTH * APT_IMG_OVERS) / 512;
                int y_scale = ceil((double)line_cnt / 512);
                image::Image<uint16_t> preview;
                preview.init(512, 512, 1);
                for (int x = 0; x < 512; x++)
                    for (int y = 0; y < line_cnt / y_scale; y++)
                    {
                        int xx = floor(double(x) * x_scale);
                        int yy = floor(double(y) * y_scale);
                        preview[y * 512 + x] = imagebuf[yy * APT_IMG_WIDTH * APT_IMG_OVERS + xx];
                    }

                ushort_to_rgba(preview.data(), textureBuffer, preview.size());
                has_to_update = true;
                last_line_cnt = line_cnt;
            }

            module_stats["unsynced_lines"] = line_cnt;
        }

        // Stop everything
        input_stream->clearReadStop();
        input_stream->clearWriteStop();
        input_stream->stopReader();
        input_stream->stopWriter();
        rtc->stop();
        frs->stop();
        rsp->stop();
        lpf->stop();
        ctm->stop();

        if (feeder_thread.joinable())
            feeder_thread.join();

        if (input_data_type == DATA_FILE)
            data_in.close();

        apt_status = PROCESSING;

        // Line mumbers
        int line_cnt = image_i / (APT_IMG_WIDTH * APT_IMG_OVERS);
        logger->info("Got %d lines...", line_cnt);

        // Buffer to image
        wip_apt_image.init(APT_IMG_WIDTH * APT_IMG_OVERS, line_cnt, 1);
        for (size_t i = 0; i < wip_apt_image.size(); i++)
            wip_apt_image[i] = imagebuf[i];

        // WB
        logger->info("White balance...");
        wip_apt_image.white_balance();

        // Save unsynced
        if (save_unsynced)
        {
            image::Image<uint16_t> wip_apt_image_sized(APT_IMG_WIDTH, line_cnt, 1);
#pragma omp parallel for
            for (int line = 0; line < line_cnt - 1; line++)
                for (int i = 0; i < APT_IMG_WIDTH; i++)
                    wip_apt_image_sized[line * APT_IMG_WIDTH + i] = wip_apt_image[line * APT_IMG_WIDTH * APT_IMG_OVERS + i * APT_IMG_OVERS];
            wip_apt_image_sized.save_img(main_dir + "/raw_unsync");
        }

        // Synchronize
        logger->info("Synchronize...");
        image::Image<uint16_t> wip_apt_image_sync = synchronize(line_cnt);

        // Parse wedges
        auto wedge_1 = wip_apt_image_sync.crop_to(996, 996 + 43);
        auto wedge_2 = wip_apt_image_sync.crop_to(2036, 2036 + 43);

        auto space_a = wip_apt_image_sync.crop_to(41, 86);
        auto space_b = wip_apt_image_sync.crop_to(1082, 1126);

        // wedge_1.save_png("wedge1.png");
        // wedge_2.save_png("wedge2.png");

        logger->trace("Wedge 1");
        auto wedges1 = parse_wedge_full(wedge_1);
        logger->trace("Wedge 2");
        auto wedges2 = parse_wedge_full(wedge_2);

        // If possible, calibrate using wedges are a reference
        int new_white = 0, new_white1 = 0;
        int new_black = 0, new_black1 = 0;
        get_calib_values_wedge(wedges1, new_white, new_black);
        get_calib_values_wedge(wedges2, new_white1, new_black1);

        APTWedge calib_wedge_ch1, calib_wedge_ch2; // We also extract calibration words, scaled to 10-bits
        uint16_t prt_counts[4];
        int space_av = 0, space_av1 = 0, space_bv = 0;
        int bb_a = 0, bb_a1 = 0;
        int channel_a = -1, channel_a1 = -1;
        int channel_b = -1;
        int switchy = -1;

        if (new_white != 0 && new_black != 0 && new_white1 != 0 && new_black1 != 0)
        {
            logger->info("Calibrating...");

            new_white = (new_white + new_white1) / 2;
            new_black = (new_black + new_black1) / 2;

            for (size_t l = 0; l < wip_apt_image_sync.height(); l++)    // Calib image
                for (size_t x = 0; x < wip_apt_image_sync.width(); x++) // for (int x = 86; x < 86 + 909; x++)
                    scale_val(wip_apt_image_sync[l * wip_apt_image_sync.width() + x], new_black, new_white);

            int validn1 = 0, validn1_1 = 0, validn1_0 = 0;
            // for (auto &wed : wedges1)
            for (unsigned int i = 0; i < wedges1.size(); i++)
            { // Calib wedges 1
                auto &wed = wedges1[i];
                scale_val(wed.ref1, new_black, new_white);
                scale_val(wed.ref2, new_black, new_white);
                scale_val(wed.ref3, new_black, new_white);
                scale_val(wed.ref4, new_black, new_white);
                scale_val(wed.ref5, new_black, new_white);
                scale_val(wed.ref6, new_black, new_white);
                scale_val(wed.ref7, new_black, new_white);
                scale_val(wed.ref8, new_black, new_white);
                scale_val(wed.zero_mod_ref, new_black, new_white);
                scale_val(wed.therm_temp1, new_black, new_white);
                scale_val(wed.therm_temp2, new_black, new_white);
                scale_val(wed.therm_temp3, new_black, new_white);
                scale_val(wed.therm_temp4, new_black, new_white);
                scale_val(wed.patch_temp, new_black, new_white);
                scale_val(wed.back_scan, new_black, new_white);
                scale_val(wed.channel, new_black, new_white);

                if (wed.max_diff < 18e3)
                {
                    if (channel_a == -1)
                    {
                        channel_a = wed.rchannel;
                    }
                    else if (channel_a1 == -1)
                    {
                        if (channel_a != wed.rchannel)
                        {
                            channel_a1 = wed.rchannel;
                            switchy = wed.start_line;
                        }
                    }
                }

                if (wed.max_diff < MAX_WEDGE_DIFF_VALID)
                {
                    calib_wedge_ch1.therm_temp1 += wed.therm_temp1;
                    calib_wedge_ch1.therm_temp2 += wed.therm_temp2;
                    calib_wedge_ch1.therm_temp3 += wed.therm_temp3;
                    calib_wedge_ch1.therm_temp4 += wed.therm_temp4;
                    calib_wedge_ch1.patch_temp += wed.patch_temp;

                    if (channel_a1 == -1)
                    {
                        bb_a += wed.back_scan;
                        validn1_0++;
                    }
                    else
                    {
                        bb_a1 += wed.back_scan;
                        validn1_1++;
                    }

                    validn1++;
                }
            }

            calib_wedge_ch1.therm_temp1 = (calib_wedge_ch1.therm_temp1 / validn1) >> 6;
            calib_wedge_ch1.therm_temp2 = (calib_wedge_ch1.therm_temp2 / validn1) >> 6;
            calib_wedge_ch1.therm_temp3 = (calib_wedge_ch1.therm_temp3 / validn1) >> 6;
            calib_wedge_ch1.therm_temp4 = (calib_wedge_ch1.therm_temp4 / validn1) >> 6;
            calib_wedge_ch1.patch_temp = (calib_wedge_ch1.patch_temp / validn1) >> 6;
            bb_a = (validn1_0 == 0 ? 0 : (bb_a / validn1_0)) >> 6;
            if (validn1_1 != 0)
                bb_a1 = (bb_a1 / validn1_1) >> 6;

            if (channel_a >= 1)
            {
                if (channel_a > 2)
                    channel_a++;
                channel_a--;
                // logger->critical("CHA %d", channel_a);
            }

            if (channel_a1 >= 1)
            {
                if (channel_a1 > 2)
                    channel_a1++;
                channel_a1--;
                // logger->critical("CHA1 %d", channel_a1);
            }

            int validn2 = 0, validn2_0 = 0;
            for (auto &wed : wedges2)
            { // Calib wedges 2
                scale_val(wed.ref1, new_black, new_white);
                scale_val(wed.ref2, new_black, new_white);
                scale_val(wed.ref3, new_black, new_white);
                scale_val(wed.ref4, new_black, new_white);
                scale_val(wed.ref5, new_black, new_white);
                scale_val(wed.ref6, new_black, new_white);
                scale_val(wed.ref7, new_black, new_white);
                scale_val(wed.ref8, new_black, new_white);
                scale_val(wed.zero_mod_ref, new_black, new_white);
                scale_val(wed.therm_temp1, new_black, new_white);
                scale_val(wed.therm_temp2, new_black, new_white);
                scale_val(wed.therm_temp3, new_black, new_white);
                scale_val(wed.therm_temp4, new_black, new_white);
                scale_val(wed.patch_temp, new_black, new_white);
                scale_val(wed.back_scan, new_black, new_white);
                scale_val(wed.channel, new_black, new_white);

                if (wed.max_diff < 18e3)
                {
                    if (channel_b == -1)
                        channel_b = wed.rchannel;
                }

                if (wed.max_diff < MAX_WEDGE_DIFF_VALID)
                {
                    calib_wedge_ch2.therm_temp1 += wed.therm_temp1;
                    calib_wedge_ch2.therm_temp2 += wed.therm_temp2;
                    calib_wedge_ch2.therm_temp3 += wed.therm_temp3;
                    calib_wedge_ch2.therm_temp4 += wed.therm_temp4;
                    calib_wedge_ch2.patch_temp += wed.patch_temp;
                    calib_wedge_ch2.back_scan += wed.back_scan;
                    validn2++;
                }
            }

            calib_wedge_ch2.therm_temp1 = (calib_wedge_ch2.therm_temp1 / validn2) >> 6;
            calib_wedge_ch2.therm_temp2 = (calib_wedge_ch2.therm_temp2 / validn2) >> 6;
            calib_wedge_ch2.therm_temp3 = (calib_wedge_ch2.therm_temp3 / validn2) >> 6;
            calib_wedge_ch2.therm_temp4 = (calib_wedge_ch2.therm_temp4 / validn2) >> 6;
            calib_wedge_ch2.patch_temp = (calib_wedge_ch2.patch_temp / validn2) >> 6;
            calib_wedge_ch2.back_scan = (calib_wedge_ch2.back_scan / validn2) >> 6;

            if (channel_b >= 1)
            {
                if (channel_b > 2)
                    channel_b++;
                channel_b--;
                // logger->critical("CHB %d", channel_b);
            }

            prt_counts[0] = (calib_wedge_ch1.therm_temp1 + calib_wedge_ch2.therm_temp1) / 2;
            prt_counts[1] = (calib_wedge_ch1.therm_temp2 + calib_wedge_ch2.therm_temp2) / 2;
            prt_counts[2] = (calib_wedge_ch1.therm_temp3 + calib_wedge_ch2.therm_temp3) / 2;
            prt_counts[3] = (calib_wedge_ch1.therm_temp4 + calib_wedge_ch2.therm_temp4) / 2;

            int validl1 = 0, validl2 = 0, validl1_1 = 0;

            if (channel_a1 != -1)
            {
                auto space_a1 = space_a;
                logger->info("%d, %d", switchy, space_a1.height());
                space_a1.crop(0, switchy - 1, space_a1.width(), space_a1.height());

                for (unsigned int y = 0; y < space_a1.height() - 1; y++)
                {
                    int min = space_a1[y * space_a1.width()], max = space_a1[y * space_a1.width()], avg = 0;
                    for (unsigned int i = 0; i < space_a1.width(); i++)
                    {
                        int v = space_a1[y * space_a1.width() + i];
                        if (v < min)
                            min = v;
                        if (v > max)
                            max = v;
                        avg += v;
                    }
                    avg /= space_a1.width();
                    scale_val(avg, new_black, new_white);
                    int max_diff = max - min;
                    if (max_diff < MAX_WEDGE_DIFF_VALID && avg != 0 && avg != 65535)
                    {
                        space_av1 += avg;
                        validl1_1++;
                    }
                }
                space_av1 /= validl1_1;
            }

            for (unsigned int y = 0; y < (channel_a1 != -1 ? switchy : space_a.height() - 1); y++)
            {
                int min = space_a[y * space_a.width()], max = space_a[y * space_a.width()], avg = 0;
                for (unsigned int i = 0; i < space_a.width(); i++)
                {
                    int v = space_a[y * space_a.width() + i];
                    if (v < min)
                        min = v;
                    if (v > max)
                        max = v;
                    avg += v;
                }
                avg /= space_a.width();
                scale_val(avg, new_black, new_white);
                int max_diff = max - min;
                if (max_diff < MAX_WEDGE_DIFF_VALID && avg != 0 && avg != 65535)
                {
                    space_av += avg;
                    validl1++;
                }
            }

            for (unsigned int y = 0; y < space_b.height(); y++)
            {
                int min = space_b[y * space_b.width()], max = space_b[y * space_b.width()], avg = 0;
                for (unsigned int i = 0; i < space_b.width(); i++)
                {
                    int v = space_b[y * space_b.width() + i];
                    if (v < min)
                        min = v;
                    if (v > max)
                        max = v;
                    avg += v;
                }
                avg /= space_b.width();
                scale_val(avg, new_black, new_white);
                int max_diff = max - min;
                if (max_diff < MAX_WEDGE_DIFF_VALID && avg != 0 && avg != 65535)
                {
                    space_bv += avg;
                    validl2++;
                }
            }

            space_av /= validl1;
            space_bv /= validl2;
        }
        else
        {
            logger->critical("Couldn't calibrate image with wedges! Likely no valid wedge was identified!");
        }

        int first_valid_line = 0;
        int last_valid_line = wip_apt_image_sync.height();

        // Save RAW before we crop
        wip_apt_image_sync.save_img(main_dir + "/raw_sync");

        if (d_autocrop_wedges)
        {
            logger->info("Autocropping using wedges...");

            int first_valid_wedge = 1e9;
            int last_valid_wedge = 0;

            for (size_t i = 0; i < wedges1.size(); i++)
            {
                if (wedges1[i].max_diff < 30e3)
                {
                    if (first_valid_wedge > wedges1[i].start_line)
                        first_valid_wedge = wedges1[i].start_line;
                    if (last_valid_wedge < wedges1[i].end_line)
                        last_valid_wedge = wedges1[i].end_line;
                }
            }

            for (size_t i = 0; i < wedges2.size(); i++)
            {
                if (wedges2[i].max_diff < 30e3)
                {
                    if (first_valid_wedge > wedges2[i].start_line)
                        first_valid_wedge = wedges2[i].start_line;
                    if (last_valid_wedge < wedges2[i].end_line)
                        last_valid_wedge = wedges2[i].end_line;
                }
            }

            logger->trace("Valid lines %d %d", first_valid_wedge, last_valid_wedge);

            if (abs(first_valid_wedge - last_valid_wedge) > 0 && first_valid_wedge != 1e9)
            {
                first_valid_line = first_valid_wedge;
                last_valid_line = last_valid_wedge;
                wip_apt_image_sync.crop(0, first_valid_line,
                                        wip_apt_image_sync.width(), last_valid_line);
                if (switchy != -1)
                    switchy -= first_valid_line;
            }
            else
            {
                logger->error("Not enough valid wedges to autocrop!");
            }
        }

        // Save
        apt_status = SAVING;

        // Products ARE not yet being processed properly. Need to parse the wedges!
        int norad = 0;
        std::string sat_name = "NOAA";
        std::optional<satdump::TLE> satellite_tle;

        if (d_parameters.contains("satellite_number") || autodetected_sat != -1)
        {
            double number = 0;

            if (autodetected_sat == -1)
            {
                try
                {
                    number = d_parameters["satellite_number"];
                }
                catch (std::exception &)
                {
                    number = std::stoi(d_parameters["satellite_number"].get<std::string>());
                }
            }
            else
            {
                number = autodetected_sat;
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
        else
        {
            logger->warn("Could not get satellite number from parameters or filename!");
        }

        double start_tt = -1;
        if (d_parameters.contains("start_timestamp") && norad != 0)
            start_tt = d_parameters["start_timestamp"];

        satdump::ProductDataSet dataset;
        dataset.satellite_name = sat_name;
        dataset.timestamp = start_tt;

        if (sat_name == "NOAA-15" && start_tt != -1)
        {
            time_t noaa15_age = dataset.timestamp - 895074720;
            int seconds = noaa15_age % 60;
            int minutes = (noaa15_age % 3600) / 60;
            int hours = (noaa15_age % 86400) / 3600;
            int days = noaa15_age / 86400;
            logger->warn("Congratulations for receiving NOAA 15 on APT! It has been %d days, %d hours, %d minutes and %d seconds since it has been launched.", days, hours, minutes, seconds);
            if (dataset.timestamp > 0)
            {
                time_t tttime = dataset.timestamp;
                std::tm *timeReadable = gmtime(&tttime);
                if (timeReadable->tm_mday == 13 && timeReadable->tm_mon == 4)
                    logger->critical("Happy birthday NOAA 15! You are now %d years old", timeReadable->tm_year + 1900 - 1998 + 1);
            }
        }

        // AVHRR
        {
            std::string names[6] = {"1", "2", "3a", "3b", "4", "5"};
            logger->info("----------- AVHRR");
            logger->info("Lines : " + std::to_string(line_cnt));
            if (channel_a1 != -1 && channel_a != -1 && channel_b != -1)
                logger->info("Identified channels: A: %s, %s B: %s", names[channel_a].c_str(), names[channel_a1].c_str(), names[channel_b].c_str());
            else if (channel_a != -1 && channel_b != -1)
                logger->info("Identified channels: A: %s B: %s", names[channel_a].c_str(), names[channel_b].c_str());
            else
                logger->warn("Identified channels (debug): A: %d A1: %d B: %d", channel_a, channel_a1, channel_b);

            satdump::ImageProducts avhrr_products;
            avhrr_products.instrument_name = "avhrr_3";
            avhrr_products.bit_depth = 8;

            image::Image<uint16_t> cha, cha1, cha2, chb;
            cha = wip_apt_image_sync.crop_to(86, 86 + 909);
            chb = wip_apt_image_sync.crop_to(1126, 1126 + 909);

            if (channel_a1 != -1)
            {
                cha1 = image::Image<uint16_t>(cha);
                cha2 = image::Image<uint16_t>(cha);
                for (unsigned int i = switchy; i < cha2.height(); i++)
                    for (unsigned int x = 0; x < cha2.width(); x++)
                        cha2[i * cha2.width() + x] = 0;

                for (int i = 0; i < switchy; i++)
                    for (unsigned int x = 0; x < cha1.width(); x++)
                        cha1[i * cha1.width() + x] = 0;
            }

            // CALIBRATION
            {
                nlohmann::json calib_coefs = loadJsonFile(resources::getResourcePath("calibration/AVHRR.json"));
                nlohmann::json calib_coefsc = calib_coefs;
                nlohmann::json calib_out;
                if (calib_coefs.contains(sat_name))
                {
                    calib_coefs = calib_coefs[sat_name];

                    // read params
                    std::vector<std::vector<double>> prt_coefs = calib_coefs["PRT"].get<std::vector<std::vector<double>>>();

                    for (int p = 0; p < 6; p++)
                    {
                        calib_out["vars"]["perChannel"][p] = calib_coefs["channels"][p];
                    }
                    for (int p = 0; p < 6; p++)
                        calib_out["wavenumbers"][p] = calib_coefs["channels"][p]["wavnb"].get<double>();

                    calib_out["wavenumbers"][6] = -1;
                    calib_out["wavenumbers"][7] = -1;

                    // calib_out["lua"] = loadFileToString(resources::getResourcePath("calibration/AVHRR.lua"));
                    calib_out["calibrator"] = "noaa_avhrr3";

                    // PRT counts to temperature
                    double tbb = 0;
                    for (int prt = 0; prt < 4; prt++)
                        for (int p = 0; p < 4; p++)
                            tbb += prt_coefs[prt][p] * pow(prt_counts[prt], p); // convert PRT counts to temperature
                    tbb /= 4;
                    for (int c = 3; c < 6; c++)
                    {
                        if (c == channel_a)
                        {
                            calib_out["vars"]["perChannel"][c]["Spc"] = space_av >> 8;
                            calib_out["vars"]["perChannel"][c]["Blb"] = bb_a >> 2;
                            calib_out["vars"]["perChannel"][c]["Nbb"] =
                                temperature_to_radiance(calib_coefs["channels"][c]["A"].get<double>() + calib_coefs["channels"][c]["B"].get<double>() * tbb,
                                                        calib_coefs["channels"][c]["Vc"].get<double>());
                        }
                        else if (c == channel_a1)
                        {
                            calib_out["vars"]["perChannel"][c]["Spc"] = space_av1 >> 8;
                            calib_out["vars"]["perChannel"][c]["Blb"] = bb_a1 >> 2;
                            calib_out["vars"]["perChannel"][c]["Nbb"] =
                                temperature_to_radiance(calib_coefs["channels"][c]["A"].get<double>() + calib_coefs["channels"][c]["B"].get<double>() * tbb,
                                                        calib_coefs["channels"][c]["Vc"].get<double>());
                        }
                        else if (c == channel_b)
                        {
                            calib_out["vars"]["perChannel"][c]["Spc"] = space_bv >> 8;
                            calib_out["vars"]["perChannel"][c]["Blb"] = calib_wedge_ch2.back_scan >> 2;
                            calib_out["vars"]["perChannel"][c]["Nbb"] =
                                temperature_to_radiance(calib_coefs["channels"][c]["A"].get<double>() + calib_coefs["channels"][c]["B"].get<double>() * tbb,
                                                        calib_coefs["channels"][c]["Vc"].get<double>());
                        }
                    }
                    avhrr_products.set_calibration(calib_out);
                    for (int n = 0; n < 3; n++)
                    {
                        avhrr_products.set_calibration_type(n, avhrr_products.CALIB_REFLECTANCE);
                        avhrr_products.set_calibration_type(n + 3, avhrr_products.CALIB_RADIANCE);
                    }
                    for (int c = 0; c < 6; c++)
                        avhrr_products.set_calibration_default_radiance_range(c, calib_coefsc["all"]["default_display_range"][c][0].get<double>(), calib_coefsc["all"]["default_display_range"][c][1].get<double>());
                }
                else
                    logger->warn("(AVHRR) Calibration data for " + sat_name + " not found. Calibration will not be performed");
            }

#if 0
            for (int i = 0; i < 6; i++)
                avhrr_products.images.push_back({"AVHRR-" + names[i], names[i], i == channel_a ? (channel_a1 == -1 ? cha : cha2) : (i == channel_b ? chb : (i == channel_a1 ? cha1 : hold))});
#else
            if (channel_a != -1)
            {
                avhrr_products.images.push_back({"AVHRR-" + names[channel_a], names[channel_a], (channel_a1 == -1 ? cha : cha2), {}, -1, -1, 0, channel_a});
            }
            if (channel_a1 != -1)
            {
                avhrr_products.images.push_back({"AVHRR-" + names[channel_a1], names[channel_a1], cha1, {}, -1, -1, 0, channel_a1});
            }
            if (channel_b != -1)
            {
                avhrr_products.images.push_back({"AVHRR-" + names[channel_b], names[channel_b], chb, {}, -1, -1, 0, channel_b});
            }
#endif

            avhrr_products.images.push_back({"APT-A", "a", cha, {}, -1, -1, 0, -2});
            avhrr_products.images.push_back({"APT-B", "b", chb, {}, -1, -1, 0, -2});

            if (d_parameters.contains("start_timestamp") && norad != 0)
            {
                if (start_tt != -1)
                {
                    std::vector<double> timestamps;

                    for (int i = first_valid_line; i < last_valid_line; i++)
                        timestamps.push_back(start_tt + (double(i) * 0.5));

                    avhrr_products.has_timestamps = true;
                    avhrr_products.set_tle(satellite_tle);
                    avhrr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                    avhrr_products.set_timestamps(timestamps);
                    nlohmann::json proj_cfg;
                    if (norad == 25338)
                        proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/noaa_15_avhrr.json"));
                    else if (norad == 28654)
                        proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/noaa_18_avhrr.json"));
                    else if (norad == 33591)
                        proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/noaa_19_avhrr.json"));
                    proj_cfg["type"] = "noaa_apt_single_line";
                    proj_cfg["image_width"] = 909;
                    proj_cfg["gcp_spacing_x"] = 30;
                    proj_cfg["gcp_spacing_y"] = 30;
                    proj_cfg.erase("corr_width");
                    proj_cfg.erase("corr_swath");
                    proj_cfg.erase("corr_resol");
                    proj_cfg.erase("corr_altit");
                    avhrr_products.set_proj_cfg(proj_cfg);
                }
            }

            avhrr_products.save(main_dir);
            dataset.products_list.push_back(".");
        }

        apt_status = DONE;

        dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        d_output_files.push_back(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/dataset.json");
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

#pragma omp parallel for
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

            // logger->critical("Line %d Pos %d Cor %d", line, best_pos, best_cor);
            for (int i = 0; i < APT_IMG_WIDTH; i++)
                wip_apt_image_sync[line * APT_IMG_WIDTH + i] = wip_apt_image[line * APT_IMG_WIDTH * APT_IMG_OVERS + best_pos + i * APT_IMG_OVERS];
        }

        return wip_apt_image_sync;
    }

    std::vector<APTWedge> NOAAAPTDecoderModule::parse_wedge_full(image::Image<uint16_t> &wedge)
    {
        if (wedge.height() < 128)
            return std::vector<APTWedge>();

        std::vector<uint8_t> sync_wedge = {31, 63, 95, 127, 159, 191, 224, 255, 0};
        std::vector<APTWedge> wedges;

        std::vector<int> final_sync_wedge;
        for (int i = 0; i < 9; i++)
            for (int f = 0; f < 8; f++)
                final_sync_wedge.push_back(sync_wedge[i]);

        std::vector<uint16_t> wedge_a;
        for (size_t line = 0; line < wedge.height(); line++)
        {
            int val = 0;
            for (int x = 0; x < 43; x++)
                val += wedge[line * wedge.width() + x];
            val /= 43;
            wedge_a.push_back(val);
        }

        for (size_t line = 0; line < (wedge_a.size() - (9 * 8)) / (16 * 8); line++)
        {
            int best_cor = 160 * 255;
            int best_pos = 0;
            for (int pos = 0; pos < 16 * 8; pos++)
            {
                int cor = 0;
                for (int i = 0; i < 9 * 8; i++)
                {
                    cor += abs(int((wedge_a[line * 16 * 8 + pos + i] >> 8) - final_sync_wedge[i]));
                }

                if (cor < best_cor)
                {
                    best_cor = cor;
                    best_pos = pos;
                }
            }

            uint16_t final_wedge[16];

            for (int i = 0; i < 16; i++)
            {
                int val = 0;
                for (int v = 0; v < 8; v++)
                    val += wedge_a[line * 16 * 8 + best_pos + i * 8 + v];
                val /= 8;
                final_wedge[i] = val;
            }

            APTWedge wed;

            wed.start_line = line * 16 * 8 + best_pos;
            wed.end_line = (line + 1) * 16 * 8 + best_pos;

            wed.ref1 = final_wedge[0];
            wed.ref2 = final_wedge[1];
            wed.ref3 = final_wedge[2];
            wed.ref4 = final_wedge[3];
            wed.ref5 = final_wedge[4];
            wed.ref6 = final_wedge[5];
            wed.ref7 = final_wedge[6];
            wed.ref8 = final_wedge[7];
            wed.zero_mod_ref = final_wedge[8];
            wed.therm_temp1 = final_wedge[9];
            wed.therm_temp2 = final_wedge[10];
            wed.therm_temp3 = final_wedge[11];
            wed.therm_temp4 = final_wedge[12];
            wed.patch_temp = final_wedge[13];
            wed.back_scan = final_wedge[14];
            wed.channel = final_wedge[15];

            if (wed.end_line < (int)wedge.height())
            {
                wed.max_diff = 0;
                for (int c = 0; c < 16; c++)
                {
                    int max_point = 0;
                    int min_point = 1e9;
                    for (int x = 0; x < 43; x++)
                    {
                        for (int y = 0; y < 8; y++)
                        {
                            int v = wedge[(wed.start_line + c * 8 + y) * wedge.width() + x];
                            if (max_point < v)
                                max_point = v;
                            if (min_point > v)
                                min_point = v;
                        }
                    }
                    // logger->debug("Max %d Min %d Diff %d",
                    //               max_point, min_point, max_point - min_point);
                    wed.max_diff += max_point - min_point;
                }
                wed.max_diff /= 16;
                // logger->trace("Diff %d", wed.max_diff);
            }
            else
            {
                wed.max_diff = 1e7;
            }

            /////////////////////////////////////
            int min_diff = 1e9;
            int best_wedge = 0;
            for (int i = 0; i < 8; i++)
            {
                int diff = abs((int)final_wedge[i] - (int)wed.channel);
                if (min_diff > diff)
                {
                    best_wedge = i + 1;
                    min_diff = diff;
                }
            }
            /////////////////////////////////////

            logger->trace("Wedge %d Pos %d Cor %d CAL %d %d %d %d %d %d %d %d CHV %d Diff %d CH %d VALID %d",
                          line, best_pos, best_cor,
                          wed.ref1, wed.ref2, wed.ref3, wed.ref4, wed.ref5, wed.ref6, wed.ref7, wed.ref8,
                          wed.channel, wed.max_diff,
                          best_wedge,
                          int(wed.max_diff < MAX_WEDGE_DIFF_VALID));

            if (1 <= best_wedge && best_wedge <= 5)
                wed.rchannel = best_wedge;

            wedges.push_back(wed);
        }

        return wedges;
    }

    void NOAAAPTDecoderModule::get_calib_values_wedge(std::vector<APTWedge> &wedges, int &new_white, int &new_black)
    {
        std::vector<uint16_t> calib_white;
        std::vector<uint16_t> calib_black;

        for (auto &wedge : wedges)
        {
            if (wedge.max_diff < MAX_WEDGE_DIFF_VALID)
            {
                calib_white.push_back(wedge.ref8);
                calib_black.push_back(wedge.zero_mod_ref);
            }
        }

        new_white = 0;
        if (calib_white.size() > 0) // At least 1 wedge "valid"
        {
            for (auto &v : calib_white)
                new_white += v;
            new_white /= calib_white.size();
        }

        new_black = 0;
        if (calib_black.size() > 0) // At least 1 wedge "valid"
        {
            for (auto &v : calib_black)
                new_black += v;
            new_black /= calib_black.size();
        }
    }

    void NOAAAPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA APT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            if (textureID == 0)
            {
                textureID = makeImageTexture();
                textureBuffer = new uint32_t[262144]; // 512x512
                memset(textureBuffer, 0, sizeof(uint32_t) * 262144);
                has_to_update = true;
            }

            if (has_to_update)
            {
                updateImageTexture(textureID, textureBuffer, 512, 512);
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
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

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