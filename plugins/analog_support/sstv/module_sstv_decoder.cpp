#include "module_sstv_decoder.h"
#include "common/dsp/filter/firdes.h"
#include "image/image.h"
#include "image/io.h"
#include "common/utils.h"
#include "common/wav.h"
#include "core/exception.h"
#include "dsp/filter/fir.h"
#include "dsp/sstv_linesync.h"
#include "dsp/utils/hilbert.h"
#include "dsp/utils/quadrature_demod.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "core/resources.h"

#include "lineproc.h"

namespace sstv
{
    SSTVDecoderModule::SSTVDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        if (parameters.count("audio_samplerate") > 0)
            d_audio_samplerate = parameters["audio_samplerate"].get<long>();
        else
            throw satdump_exception("Audio samplerate parameter must be present!");

        if (parameters.count("sstv_mode") > 0)
            d_sstv_mode = parameters["sstv_mode"].get<std::string>();
        else
            throw satdump_exception("SSTV Mode parameter must be present!");
    }

    SSTVDecoderModule::~SSTVDecoderModule() {}

    void SSTVDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        std::string main_dir = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        bool is_stereo = false;

        std::ifstream data_in;
        if (input_data_type == DATA_FILE)
        {
            wav::WavHeader hdr = wav::parseHeaderFromFileWav(d_input_file);
            if (!wav::isValidWav(hdr))
                throw satdump_exception("File is not WAV!");
            if (hdr.bits_per_sample != 16)
                throw satdump_exception("Only 16-bits WAV are supported!");
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

            data_in = std::ifstream(d_input_file, std::ios::binary);
        }

        logger->info("Using input wav " + d_input_file);

        // SSTV Blocks
        satdump::ndsp::FIRBlock<float> bpf;
        satdump::ndsp::HilbertBlock hilb;
        satdump::ndsp::QuadratureDemodBlock quad;
        satdump::ndsp::SSTVLineSyncBlock lsync;

        // Configure blocks
        auto sstv_modes = loadJsonFile(resources::getResourcePath("sstv.json"));

        if (!sstv_modes.contains(d_sstv_mode))
            throw satdump_exception("Invalid SSTV Mode : " + d_sstv_mode);
        nlohmann::json sstv_cfg = sstv_modes[d_sstv_mode];

        double sstv_samplerate = sstv_cfg["samplerate"]; // TODO resample!
        double freq_sync = getValueOrDefault(sstv_cfg["sync_freq"], 1200);
        double freq_img_black = getValueOrDefault(sstv_cfg["freq_img_black"], 1488); // 1500); should be 1500?
        double freq_img_white = getValueOrDefault(sstv_cfg["freq_img_white"], 2300);

        bpf.set_cfg("taps", dsp::firdes::band_pass(1, sstv_samplerate, freq_sync, freq_img_white, 500, dsp::fft::window::WIN_KAISER));

        hilb.set_cfg("ntaps", 651);

        lsync.set_cfg("sync_time", sstv_cfg["sync_time"]);
        lsync.set_cfg("line_time", sstv_cfg["line_time"]);

        // Link them
        satdump::ndsp::BlockIO io_in;
        io_in.name = "in";
        io_in.type = satdump::ndsp::DSP_SAMPLE_TYPE_F32;
        io_in.fifo = std::make_shared<satdump::ndsp::DspBufferFifo>(2);

        bpf.set_input(io_in, 0);
        hilb.link(&bpf, 0, 0, 2);
        quad.link(&hilb, 0, 0, 2);
        lsync.link(&quad, 0, 0, 2);

        auto io_out = lsync.get_output(0, 4);

        // Start everything
        auto feeder_func = [this, &io_in, &data_in, is_stereo]()
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

                auto nblk = satdump::ndsp::DSPBuffer::newBufferSamples<float>(buffer_size);

                if (is_stereo)
                {
                    for (int i = 0; i < buffer_size / 2; i++)
                        nblk.getSamples<float>()[i] = s16_buf[i * 2] / 32767.0f;
                    nblk.size = buffer_size / 2;
                    io_in.fifo->wait_enqueue(nblk);
                }
                else
                {
                    volk_16i_s32f_convert_32f_u(nblk.getSamples<float>(), (const int16_t *)s16_buf, 32767, buffer_size);
                    nblk.size = buffer_size;
                    io_in.fifo->wait_enqueue(nblk);
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
            io_in.fifo->wait_enqueue(satdump::ndsp::DSPBuffer::newBufferTerminator());
        };

        std::thread feeder_thread(feeder_func);

        bpf.start();
        hilb.start();
        quad.start();
        lsync.start();

        std::vector<uint32_t> all_lines;
        SSTVLineProc lproc;

        lproc.setCfg(sstv_cfg);

        auto reader_func = [&]()
        {
            while (true)
            {
                satdump::ndsp::DSPBuffer iblk;
                io_out.fifo->wait_dequeue(iblk);

                if (iblk.isTerminator())
                {
                    iblk.free();
                    break;
                }

                //                logger->info("Got Something!");

                auto ll = lproc.lineProc(std::vector<float>(iblk.getSamples<float>(), iblk.getSamples<float>() + iblk.size));
                all_lines.insert(all_lines.end(), ll.begin(), ll.end());

                if (textureID != 0 && all_lines.size() > 0)
                {
                    auto img = rgbaToImg(all_lines, lproc.output_width);

                    img.resize_bilinear(512, img.height() * (512.0 / img.width()));

                    image::Image preview(8, 512, 512, 3);

                    int offset = img.height() > 512 ? (img.height() - 512) : 0;
                    for (int ch = 0; ch < img.channels(); ch++)
                        for (int i = 0; i < std::min<int>(512, img.height()); i++)
                            image::imemcpy(preview, ch * preview.height() * preview.width() + i * 512, img, ch * img.height() * img.width() + (i + offset) * 512, 512);

                    image::image_to_rgba(preview, textureBuffer);
                    has_to_update = true;
                }

                iblk.free();
            }

            logger->info("BYE!");
        };

        std::thread read_thread(reader_func);

        // Stop everything
        if (feeder_thread.joinable())
            feeder_thread.join();

        bpf.stop();
        hilb.stop();
        quad.stop();
        lsync.stop();

        if (read_thread.joinable())
            read_thread.join();

        auto img = rgbaToImg(all_lines, lproc.output_width);
        image::save_img(img, d_output_file_hint + ".png");
    }

    void SSTVDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("SSTV Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
            drawStatus(sstv_status);
        }
        ImGui::EndGroup();

        if (input_data_type == DATA_FILE)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string SSTVDecoderModule::getID() { return "sstv_decoder"; }

    std::vector<std::string> SSTVDecoderModule::getParameters() { return {}; }

    std::shared_ptr<ProcessingModule> SSTVDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SSTVDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace sstv