#include "module_meteor_hrpt_demod.h"
#include <dsp/fir_gen.h>
#include "logger.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    METEORHRPTDemodModule::METEORHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                        d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                                        d_buffer_size(std::stoi(parameters["buffer_size"]))
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
        agc = std::make_shared<libdsp::AgcCC>(0.0038e-3f, 1.0f, 0.5f / 32768.0f, 65536);
        rrc = std::make_shared<libdsp::FIRFilterCCF>(1, libdsp::firgen::root_raised_cosine(1, d_samplerate, 665400.0f * 2.2f, 0.5f, 31));
        pll = std::make_shared<libdsp::BPSKCarrierPLL>(0.030f, powf(0.030f, 2) / 4.0f, 0.5f);
        mov = std::make_shared<libdsp::MovingAverageFF>(round(((float)d_samplerate / (float)665400) / 2.0f), 1.0 / round(((float)d_samplerate / (float)665400) / 2.0f), d_buffer_size, 1);
        rec = std::make_shared<libdsp::ClockRecoveryMMFF>(((float)d_samplerate / (float)665400) / 2.0f, powf(40e-3, 2) / 4.0f, 1.0f, 40e-3, 0.01f);

        // Buffers
        in_buffer = new std::complex<float>[d_buffer_size];
        in_buffer2 = new std::complex<float>[d_buffer_size];
        agc_buffer = new std::complex<float>[d_buffer_size];
        agc_buffer2 = new std::complex<float>[d_buffer_size];
        rrc_buffer = new std::complex<float>[d_buffer_size];
        rrc_buffer2 = new std::complex<float>[d_buffer_size];
        pll_buffer = new float[d_buffer_size];
        pll_buffer2 = new float[d_buffer_size];
        mov_buffer = new float[d_buffer_size];
        mov_buffer2 = new float[d_buffer_size];
        rec_buffer = new float[d_buffer_size];
        rec_buffer2 = new float[d_buffer_size];
        bits_buffer = new uint8_t[d_buffer_size * 10];

        buffer_i16 = new int16_t[d_buffer_size * 2];
        buffer_i8 = new int8_t[d_buffer_size * 2];
        buffer_u8 = new uint8_t[d_buffer_size * 2];

        // Init FIFOs
        in_pipe = new libdsp::Pipe<std::complex<float>>(d_buffer_size * 10);
        agc_pipe = new libdsp::Pipe<std::complex<float>>(d_buffer_size * 10);
        rrc_pipe = new libdsp::Pipe<std::complex<float>>(d_buffer_size * 10);
        pll_pipe = new libdsp::Pipe<float>(d_buffer_size * 10);
        mov_pipe = new libdsp::Pipe<float>(d_buffer_size * 10);
        rec_pipe = new libdsp::Pipe<float>(d_buffer_size * 10);
    }

    std::vector<ModuleDataType> METEORHRPTDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> METEORHRPTDemodModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    METEORHRPTDemodModule::~METEORHRPTDemodModule()
    {
        delete[] in_buffer;
        delete[] in_buffer2;
        delete[] agc_buffer;
        delete[] agc_buffer2;
        delete[] rrc_buffer;
        delete[] rrc_buffer2;
        delete[] pll_buffer;
        delete[] pll_buffer2;
        delete[] rec_buffer;
        delete[] rec_buffer2;
        delete[] mov_buffer;
        delete[] mov_buffer2;
        delete[] bits_buffer;
        delete[] buffer_i16;
        delete[] buffer_i8;
        delete[] buffer_u8;
        //delete[] rrc_pipe;
        //delete[] agc_pipe;
        //delete[] pll_pipe;
        //delete[] rec_pipe;
    }

    void METEORHRPTDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        data_out = std::ofstream(d_output_file_hint + ".dem", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".dem");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".dem");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        agcRun = rrcRun = pllRun = recRun = movRun = true;

        fileThread = std::thread(&METEORHRPTDemodModule::fileThreadFunction, this);
        agcThread = std::thread(&METEORHRPTDemodModule::agcThreadFunction, this);
        rrcThread = std::thread(&METEORHRPTDemodModule::rrcThreadFunction, this);
        pllThread = std::thread(&METEORHRPTDemodModule::pllThreadFunction, this);
        movThread = std::thread(&METEORHRPTDemodModule::movThreadFunction, this);
        recThread = std::thread(&METEORHRPTDemodModule::clockrecoveryThreadFunction, this);

        int dat_size = 0;
        while (!data_in.eof())
        {
            dat_size = rec_pipe->pop(rec_buffer2, d_buffer_size);

            if (dat_size <= 0)
                continue;

            volk_32f_binary_slicer_8i_generic((int8_t *)bits_buffer, rec_buffer2, dat_size);

            std::vector<uint8_t> bytes = getBytes(bits_buffer, dat_size);

            data_out.write((char *)&bytes[0], bytes.size());

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        logger->info("Demodulation finished");

        data_out.close();
        data_in.close();

        // Exit all threads... Without causing a race condition!
        agcRun = rrcRun = pllRun = recRun = movRun = false;

        in_pipe->~Pipe();

        if (fileThread.joinable())
            fileThread.join();

        logger->debug("FILE OK");

        agc_pipe->~Pipe();

        if (agcThread.joinable())
            agcThread.join();

        logger->debug("AGC OK");

        rrc_pipe->~Pipe();

        if (rrcThread.joinable())
            rrcThread.join();

        logger->debug("RRC OK");

        pll_pipe->~Pipe();

        if (pllThread.joinable())
            pllThread.join();

        logger->debug("PLL OK");

        mov_pipe->~Pipe();

        if (movThread.joinable())
            movThread.join();

        logger->debug("MOW OK");

        rec_pipe->~Pipe();

        if (recThread.joinable())
            recThread.join();

        logger->debug("REC OK");
    }

    void METEORHRPTDemodModule::fileThreadFunction()
    {
        int gotten;
        while (input_data_type == DATA_STREAM ? true : !data_in.eof())
        {
            // Get baseband, possibly convert to F32
            if (f32)
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)in_buffer, d_buffer_size * sizeof(std::complex<float>));
                else
                    gotten = input_fifo->pop((uint8_t *)in_buffer, d_buffer_size, sizeof(std::complex<float>));
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
                    in_buffer[i] = (float)buffer_i16[i * 2] + (float)buffer_i16[i * 2 + 1] * 1if;
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
                    in_buffer[i] = (float)buffer_i8[i * 2] + (float)buffer_i8[i * 2 + 1] * 1if;
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
                    in_buffer[i] = real + imag * 1if;
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();
            else
                progress = 0;

            in_pipe->push(in_buffer, d_buffer_size);
        }
    }

    void METEORHRPTDemodModule::agcThreadFunction()
    {
        int gotten;
        while (agcRun)
        {
            gotten = in_pipe->pop(in_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            /// AGC
            agc->work(in_buffer2, gotten, agc_buffer);

            agc_pipe->push(agc_buffer, gotten);
        }
    }

    void METEORHRPTDemodModule::rrcThreadFunction()
    {
        int gotten;
        while (rrcRun)
        {
            gotten = agc_pipe->pop(agc_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            // Root-raised-cosine filtering
            int out = rrc->work(agc_buffer2, gotten, rrc_buffer);

            rrc_pipe->push(rrc_buffer, out);
        }
    }

    void METEORHRPTDemodModule::pllThreadFunction()
    {
        int gotten;
        while (pllRun)
        {
            gotten = rrc_pipe->pop(rrc_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            // Costas loop, frequency offset recovery
            pll->work(rrc_buffer2, gotten, pll_buffer);

            pll_pipe->push(pll_buffer, gotten);
        }
    }

    void METEORHRPTDemodModule::movThreadFunction()
    {
        int gotten;
        while (movRun)
        {
            gotten = pll_pipe->pop(pll_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            // Clock recovery
            int out = mov->work(pll_buffer2, gotten, mov_buffer);

            mov_pipe->push(mov_buffer, out);
        }
    }

    void METEORHRPTDemodModule::clockrecoveryThreadFunction()
    {
        int gotten;
        while (recRun)
        {
            gotten = mov_pipe->pop(mov_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            int recovered_size = 0;

            try
            {
                // Clock recovery
                recovered_size = rec->work(mov_buffer2, gotten, rec_buffer);
            }
            catch (std::runtime_error &e)
            {
                logger->error(e.what());
            }

            rec_pipe->push(rec_buffer, recovered_size);
        }
    }

    void METEORHRPTDemodModule::drawUI()
    {
        ImGui::Begin("METEOR HRPT Demodulator", NULL, NOWINDOW_FLAGS);

        // Constellation
        {
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + 200, ImGui::GetCursorScreenPos().y + 200),
                                     ImColor::HSV(0, 0, 0));

            for (int i = 0; i < 2048; i++)
            {
                draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + rec_buffer2[i] * 50) % 200,
                                                  ImGui::GetCursorScreenPos().y + (int)(100 + rng.gasdev() * 6) % 200),
                                           2,
                                           ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
            }

            ImGui::Dummy(ImVec2(200 + 3, 200 + 3));
        }

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

        ImGui::End();
    }

    std::string METEORHRPTDemodModule::getID()
    {
        return "meteor_hrpt_demod";
    }

    std::vector<std::string> METEORHRPTDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format"};
    }

    std::shared_ptr<ProcessingModule> METEORHRPTDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<METEORHRPTDemodModule>(input_file, output_file_hint, parameters);
    }

    std::vector<uint8_t> METEORHRPTDemodModule::getBytes(uint8_t *bits, int length)
    {
        std::vector<uint8_t> bytesToRet;
        for (int ii = 0; ii < length; ii++)
        {
            byteToWrite = (byteToWrite << 1) | bits[ii];
            inByteToWrite++;

            if (inByteToWrite == 8)
            {
                bytesToRet.push_back(byteToWrite);
                inByteToWrite = 0;
            }
        }

        return bytesToRet;
    }

    void METEORHRPTDemodModule::volk_32f_binary_slicer_8i_generic(int8_t *cVector, const float *aVector, unsigned int num_points)
    {
        int8_t *cPtr = cVector;
        const float *aPtr = aVector;
        unsigned int number = 0;

        for (number = 0; number < num_points; number++)
        {
            if (*aPtr++ >= 0)
            {
                *cPtr++ = 1;
            }
            else
            {
                *cPtr++ = 0;
            }
        }
    }
} // namespace meteor