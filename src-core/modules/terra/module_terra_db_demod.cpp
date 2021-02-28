#include "module_terra_db_demod.h"
#include <dsp/fir_gen.h>
#include "logger.h"
#include "imgui/imgui.h"
#include <dsp/dc_blocker.h>

// Return filesize
size_t getFilesize(std::string filepath);

namespace terra
{
    TerraDBDemodModule::TerraDBDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                  d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                                  d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                                  d_dc_block(parameters.count("dc_block") > 0 ? std::stoi(parameters["dc_block"]) : 0)
    {
        if (parameters["baseband_format"] == "i16")
        {
            i16 = true;
        }
        else if (parameters["baseband_format"] == "i8")
        {
            i8 = true;
        }
        else if (parameters["baseband_format"] == "f32")
        {
            f32 = true;
        }
        else if (parameters["baseband_format"] == "w8")
        {
            w8 = true;
        }

        // Init DSP blocks
        agc = std::make_shared<libdsp::AgcCC>(1e-3, 1.0f, 1.0f, 65536);
        rrc = std::make_shared<libdsp::FIRFilterCCF>(1, libdsp::firgen::root_raised_cosine(1, d_samplerate, 13.125e6 * 2, 0.5f, 31));
        pll = std::make_shared<libdsp::CostasLoop>(0.0063, 2);
        rec = std::make_shared<libdsp::ClockRecoveryMMCC>(((float)d_samplerate / (float)13.125e6) / 2.0f, pow(0.001, 2) / 4.0, 0.5f, 0.001, 0.005f);

        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 2];
        buffer_i16 = new int16_t[d_buffer_size * 2];
        buffer_i8 = new int8_t[d_buffer_size * 2];
        buffer_u8 = new uint8_t[d_buffer_size * 2];
    }

    std::vector<ModuleDataType> TerraDBDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> TerraDBDemodModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    TerraDBDemodModule::~TerraDBDemodModule()
    {
        delete[] sym_buffer;
        delete[] buffer_i16;
        delete[] buffer_i8;
        delete[] buffer_u8;
    }

    void TerraDBDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".soft");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        agcRun = rrcRun = pllRun = recRun = true;

        fileThread = std::thread(&TerraDBDemodModule::fileThreadFunction, this);
        agcThread = std::thread(&TerraDBDemodModule::agcThreadFunction, this);
        rrcThread = std::thread(&TerraDBDemodModule::rrcThreadFunction, this);
        pllThread = std::thread(&TerraDBDemodModule::pllThreadFunction, this);
        recThread = std::thread(&TerraDBDemodModule::clockrecoveryThreadFunction, this);

        int dat_size = 0;
        while (input_data_type == DATA_STREAM ? input_active.load() : !data_in.eof())
        {
            dat_size = rec_pipe.read(); //->read(rec_buffer2, d_buffer_size);

            if (dat_size <= 0)
                continue;

            for (int i = 0; i < dat_size; i++)
            {
                sym_buffer[i] = clamp(rec_pipe.readBuf[i].real() * 50);
            }

            rec_pipe.flush();

            data_out.write((char *)sym_buffer, dat_size);

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        logger->info("Demodulation finished");

        if (fileThread.joinable())
            fileThread.join();

        logger->debug("FILE OK");
    }

    void TerraDBDemodModule::fileThreadFunction()
    {
        libdsp::DCBlocker dcB(320, true);
        int gotten;
        while (input_data_type == DATA_STREAM ? input_active.load() : !data_in.eof())
        {
            // Get baseband, possibly convert to F32
            if (f32)
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)in_pipe.writeBuf, d_buffer_size * sizeof(std::complex<float>));
                else
                    gotten = input_fifo->pop((uint8_t *)in_pipe.writeBuf, d_buffer_size, sizeof(std::complex<float>));
            }
            else if (i16)
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer_i16, d_buffer_size * sizeof(int16_t) * 2);
                else
                    gotten = input_fifo->pop((uint8_t *)buffer_i16, d_buffer_size, sizeof(int16_t) * 2);

                for (int i = 0; i < d_buffer_size; i++)
                {
                    using namespace std::complex_literals;
                    in_pipe.writeBuf[i] = (float)buffer_i16[i * 2] + (float)buffer_i16[i * 2 + 1] * 1if;
                }
            }
            else if (i8)
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer_i8, d_buffer_size * sizeof(int8_t) * 2);
                else
                    gotten = input_fifo->pop((uint8_t *)buffer_i8, d_buffer_size, sizeof(int8_t) * 2);

                for (int i = 0; i < d_buffer_size; i++)
                {
                    using namespace std::complex_literals;
                    in_pipe.writeBuf[i] = (float)buffer_i8[i * 2] + (float)buffer_i8[i * 2 + 1] * 1if;
                }
            }
            else if (w8)
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer_u8, d_buffer_size * sizeof(uint8_t) * 2);
                else
                    gotten = input_fifo->pop((uint8_t *)buffer_i8, d_buffer_size, sizeof(uint8_t) * 2);

                for (int i = 0; i < d_buffer_size; i++)
                {
                    float imag = (buffer_u8[i * 2] - 127) * 0.004f;
                    float real = (buffer_u8[i * 2 + 1] - 127) * 0.004f;
                    using namespace std::complex_literals;
                    in_pipe.writeBuf[i] = real + imag * 1if;
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();
            else
                progress = 0;

            if (d_dc_block)
                dcB.work(in_pipe.writeBuf, input_data_type == DATA_FILE ? d_buffer_size : gotten, in_pipe.writeBuf);

            in_pipe.swap(d_buffer_size); //->write(in_buffer, input_data_type == DATA_FILE ? d_buffer_size : gotten);
        }

        if (input_data_type == DATA_FILE)
            data_in.close();

        // Exit all threads... Without causing a race condition!
        agcRun = rrcRun = pllRun = recRun = false;

        in_pipe.stopWriter();
        in_pipe.stopReader();

        agc_pipe.stopWriter();

        if (agcThread.joinable())
            agcThread.join();

        logger->debug("AGC OK");

        agc_pipe.stopReader();
        rrc_pipe.stopWriter();

        if (rrcThread.joinable())
            rrcThread.join();

        logger->debug("RRC OK");

        rrc_pipe.stopReader();
        pll_pipe.stopWriter();

        if (pllThread.joinable())
            pllThread.join();

        logger->debug("PLL OK");

        pll_pipe.stopReader();
        rec_pipe.stopWriter();

        if (recThread.joinable())
            recThread.join();

        logger->debug("REC OK");

        data_out.close();

        rec_pipe.stopReader();
    }

    void TerraDBDemodModule::agcThreadFunction()
    {
        int gotten;
        while (agcRun)
        {
            gotten = in_pipe.read(); //->read(in_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            /// AGC
            agc->work(in_pipe.readBuf, gotten, agc_pipe.writeBuf);

            in_pipe.flush();
            agc_pipe.swap(gotten); //->write(agc_buffer, gotten);
        }
    }

    void TerraDBDemodModule::rrcThreadFunction()
    {
        int gotten;
        while (rrcRun)
        {
            gotten = agc_pipe.read(); //->read(agc_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            // Root-raised-cosine filtering
            int out = rrc->work(agc_pipe.readBuf, gotten, rrc_pipe.writeBuf);

            agc_pipe.flush();
            rrc_pipe.swap(out); //->write(rrc_buffer, out);
        }
    }

    void TerraDBDemodModule::pllThreadFunction()
    {
        int gotten;
        while (pllRun)
        {
            gotten = rrc_pipe.read(); //->read(rrc_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            // Costas loop, frequency offset recovery
            pll->work(rrc_pipe.readBuf, gotten, pll_pipe.writeBuf);

            rrc_pipe.flush();
            pll_pipe.swap(gotten); //->write(pll_buffer, gotten);
        }
    }

    void TerraDBDemodModule::clockrecoveryThreadFunction()
    {
        int gotten;
        while (recRun)
        {
            gotten = pll_pipe.read(); //->read(pll_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            int recovered_size = 0;

            try
            {
                // Clock recovery
                recovered_size = rec->work(pll_pipe.readBuf, gotten, rec_pipe.writeBuf);
            }
            catch (std::runtime_error &e)
            {
                logger->error(e.what());
            }

            pll_pipe.flush();
            rec_pipe.swap(recovered_size); //->write(rec_buffer, recovered_size);
        }
    }

    void TerraDBDemodModule::drawUI()
    {
        ImGui::Begin("Terra DB Demodulator", NULL, NOWINDOW_FLAGS);

        // Constellation
        {
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + 200, ImGui::GetCursorScreenPos().y + 200),
                                     ImColor::HSV(0, 0, 0));

            for (int i = 0; i < 2048; i++)
            {
                draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + rec_pipe.readBuf[i].real() * 50) % 200,
                                                  ImGui::GetCursorScreenPos().y + (int)(100 + rec_pipe.readBuf[i].imag() * 50) % 200),
                                           2,
                                           ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
            }

            ImGui::Dummy(ImVec2(200 + 3, 200 + 3));
        }

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

        ImGui::End();
    }

    std::string TerraDBDemodModule::getID()
    {
        return "terra_db_demod";
    }

    std::vector<std::string> TerraDBDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "dc_block", "baseband_format"};
    }

    std::shared_ptr<ProcessingModule> TerraDBDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<TerraDBDemodModule>(input_file, output_file_hint, parameters);
    }
}