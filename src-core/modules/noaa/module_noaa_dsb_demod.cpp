#include "module_noaa_dsb_demod.h"
#include <dsp/fir_gen.h>
#include "logger.h"
#include "modules/common/manchester.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace noaa
{
    NOAADSBDemodModule::NOAADSBDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
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
        agc = std::make_shared<libdsp::AgcCC>(1e-2f, 1.0f, 1.0f, 65536);
        pll = std::make_shared<libdsp::BPSKCarrierPLL>(0.01f, powf(0.01, 2) / 4.0f, 3.0f * M_PI * 100e3 / (float)d_samplerate);
        rrc = std::make_shared<libdsp::FIRFilterFFF>(1, libdsp::firgen::root_raised_cosine(1, (float)d_samplerate / 2.0f, (float)8320, 0.5, 1023));
        rec = std::make_shared<libdsp::ClockRecoveryMMFF>(((float)d_samplerate / (float)8320) / 2.0f, powf(0.01, 2) / 4.0f, 0.5f, 0.01, 100e-6);
        rep = std::make_shared<RepackBitsByte>();
        def = std::make_shared<DSBDeframer>();

        // Buffers
        in_buffer = new std::complex<float>[d_buffer_size];
        in_buffer2 = new std::complex<float>[d_buffer_size];
        agc_buffer = new std::complex<float>[d_buffer_size];
        agc_buffer2 = new std::complex<float>[d_buffer_size];
        pll_buffer = new float[d_buffer_size];
        pll_buffer2 = new float[d_buffer_size];
        rrc_buffer = new float[d_buffer_size];
        rrc_buffer2 = new float[d_buffer_size];
        rec_buffer = new float[d_buffer_size];
        rec_buffer2 = new float[d_buffer_size];
        bits_buffer = new uint8_t[d_buffer_size * 10];
        bytes_buffer = new uint8_t[d_buffer_size * 10];
        manchester_buffer = new uint8_t[d_buffer_size * 10];

        buffer_i16 = new int16_t[d_buffer_size * 2];
        buffer_i8 = new int8_t[d_buffer_size * 2];
        buffer_u8 = new uint8_t[d_buffer_size * 2];

        // Init FIFOs
        in_pipe = new libdsp::Pipe<std::complex<float>>(d_buffer_size * 10);
        agc_pipe = new libdsp::Pipe<std::complex<float>>(d_buffer_size * 10);
        pll_pipe = new libdsp::Pipe<float>(d_buffer_size * 10);
        rrc_pipe = new libdsp::Pipe<float>(d_buffer_size * 10);
        rec_pipe = new libdsp::Pipe<float>(d_buffer_size * 10);
    }

    NOAADSBDemodModule::~NOAADSBDemodModule()
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
        delete[] bits_buffer;
        delete[] buffer_i16;
        delete[] buffer_i8;
        delete[] buffer_u8;
        delete[] bytes_buffer;
        delete[] manchester_buffer;
        //delete[] agc_pipe;
        //delete[] pll_pipe;
        //delete[] rec_pipe;
    }

    void NOAADSBDemodModule::process()
    {
        size_t filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".tip", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".tip");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".tip");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        agcRun = rrcRun = pllRun = recRun = true;

        fileThread = std::thread(&NOAADSBDemodModule::fileThreadFunction, this);
        agcThread = std::thread(&NOAADSBDemodModule::agcThreadFunction, this);
        pllThread = std::thread(&NOAADSBDemodModule::pllThreadFunction, this);
        rrcThread = std::thread(&NOAADSBDemodModule::rrcThreadFunction, this);
        recThread = std::thread(&NOAADSBDemodModule::clockrecoveryThreadFunction, this);

        int frame_count = 0;

        int dat_size = 0, bytes_out = 0, num_byte = 0;
        while (!data_in.eof())
        {
            dat_size = rec_pipe->pop(rec_buffer2, d_buffer_size);

            if (dat_size <= 0)
                continue;

            volk_32f_binary_slicer_8i_generic((int8_t *)bits_buffer, rec_buffer2, dat_size);

            bytes_out = rep->work(bits_buffer, dat_size, bytes_buffer);

            defra_buf.insert(defra_buf.end(), &bytes_buffer[0], &bytes_buffer[bytes_out]);
            num_byte = defra_buf.size() - defra_buf.size() % 2;

            manchesterDecoder(&defra_buf.data()[0], num_byte, manchester_buffer);

            std::vector<std::array<uint8_t, FRAME_SIZE>> frames = def->work(manchester_buffer, num_byte / 2);

            // Count frames
            frame_count += frames.size();

            // Erase used up elements
            defra_buf.erase(defra_buf.begin(), defra_buf.begin() + num_byte);

            // Write to file
            if (frames.size() > 0)
            {
                for (std::array<uint8_t, FRAME_SIZE> frm : frames)
                {
                    data_out.write((char *)&frm[0], FRAME_SIZE);
                }
            }

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%, Deframer : " + deframer_state + ", Frames : " + std::to_string(frame_count));
            }
        }

        logger->info("Demodulation finished");

        data_out.close();
        data_in.close();

        // Exit all threads... Without causing a race condition!
        agcRun = rrcRun = pllRun = recRun = false;

        in_pipe->~Pipe();

        if (fileThread.joinable())
            fileThread.join();

        logger->debug("FILE OK");

        agc_pipe->~Pipe();

        if (agcThread.joinable())
            agcThread.join();

        logger->debug("AGC OK");

        pll_pipe->~Pipe();

        if (pllThread.joinable())
            pllThread.join();

        logger->debug("PLL OK");

        rrc_pipe->~Pipe();

        if (rrcThread.joinable())
            rrcThread.join();

        logger->debug("RRC OK");

        rec_pipe->~Pipe();

        if (recThread.joinable())
            recThread.join();

        logger->debug("REC OK");
    }

    void NOAADSBDemodModule::fileThreadFunction()
    {
        while (!data_in.eof())
        {
            // Get baseband, possibly convert to F32
            if (f32)
            {
                data_in.read((char *)in_buffer, d_buffer_size * sizeof(std::complex<float>));
            }
            else if (i16)
            {
                data_in.read((char *)buffer_i16, d_buffer_size * sizeof(int16_t) * 2);
                for (int i = 0; i < d_buffer_size; i++)
                {
                    using namespace std::complex_literals;
                    in_buffer[i] = (float)buffer_i16[i * 2] + (float)buffer_i16[i * 2 + 1] * 1if;
                }
            }
            else if (i8)
            {
                data_in.read((char *)buffer_i8, d_buffer_size * sizeof(int8_t) * 2);
                for (int i = 0; i < d_buffer_size; i++)
                {
                    using namespace std::complex_literals;
                    in_buffer[i] = (float)buffer_i8[i * 2] + (float)buffer_i8[i * 2 + 1] * 1if;
                }
            }
            else if (w8)
            {
                data_in.read((char *)buffer_u8, d_buffer_size * sizeof(uint8_t) * 2);
                for (int i = 0; i < d_buffer_size; i++)
                {
                    float imag = (buffer_u8[i * 2] - 127) * 0.004f;
                    float real = (buffer_u8[i * 2 + 1] - 127) * 0.004f;
                    using namespace std::complex_literals;
                    in_buffer[i] = real + imag * 1if;
                }
            }

            in_pipe->push(in_buffer, d_buffer_size);
        }
    }

    void NOAADSBDemodModule::agcThreadFunction()
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

    void NOAADSBDemodModule::pllThreadFunction()
    {
        int gotten;
        while (pllRun)
        {
            gotten = agc_pipe->pop(agc_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            // Costas loop, frequency offset recovery
            pll->work(agc_buffer2, gotten, pll_buffer);

            pll_pipe->push(pll_buffer, gotten);
        }
    }

    void NOAADSBDemodModule::rrcThreadFunction()
    {
        int gotten;
        while (rrcRun)
        {
            gotten = pll_pipe->pop(pll_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            // Root-raised-cosine filtering
            int out = rrc->work(pll_buffer2, gotten, rrc_buffer);

            rrc_pipe->push(rrc_buffer, out);
        }
    }

    void NOAADSBDemodModule::clockrecoveryThreadFunction()
    {
        int gotten;
        while (recRun)
        {
            gotten = rrc_pipe->pop(rrc_buffer2, d_buffer_size);

            if (gotten <= 0)
                continue;

            int recovered_size = 0;

            try
            {
                // Clock recovery
                recovered_size = rec->work(rrc_buffer2, gotten, rec_buffer);
            }
            catch (std::runtime_error &e)
            {
                logger->error(e.what());
            }

            rec_pipe->push(rec_buffer, recovered_size);
        }
    }

    std::string NOAADSBDemodModule::getID()
    {
        return "noaa_dsb_demod";
    }

    std::vector<std::string> NOAADSBDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format"};
    }

    std::shared_ptr<ProcessingModule> NOAADSBDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<NOAADSBDemodModule>(input_file, output_file_hint, parameters);
    }

    void NOAADSBDemodModule::volk_32f_binary_slicer_8i_generic(int8_t *cVector, const float *aVector, unsigned int num_points)
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
} // namespace noaa