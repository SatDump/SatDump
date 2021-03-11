#include "module_fsk_demod.h"
#include "modules/common/dsp/fir_gen.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// Return filesize
size_t getFilesize(std::string filepath);

FSKDemodModule::FSKDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                      d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                      d_symbolrate(std::stoi(parameters["symbolrate"])),
                                                                                                                                      d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                      d_lpf_cutoff(std::stoi(parameters["lpf_cutoff"])),
                                                                                                                                      d_lpf_transition_width(std::stoi(parameters["lpf_transition_width"]))
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
    lpf = std::make_shared<dsp::FIRFilterCCF>(1, dsp::firgen::low_pass(1, d_samplerate, d_lpf_cutoff, d_lpf_transition_width));
    qua = std::make_shared<dsp::QuadratureDemod>(1.0f);
    rec = std::make_shared<dsp::ClockRecoveryMMFF>((float)d_samplerate / (float)d_symbolrate, 0.25f * 0.175f * 0.175f, 0.5f, 0.175f, 0.005f);

    // Buffers
    sym_buffer = new int8_t[d_buffer_size * 10];

    buffer_i16 = new int16_t[d_buffer_size * 2];
    buffer_i8 = new int8_t[d_buffer_size * 2];
    buffer_u8 = new uint8_t[d_buffer_size * 2];
}

std::vector<ModuleDataType> FSKDemodModule::getInputTypes()
{
    return {DATA_FILE, DATA_STREAM};
}

std::vector<ModuleDataType> FSKDemodModule::getOutputTypes()
{
    return {DATA_FILE};
}

FSKDemodModule::~FSKDemodModule()
{
    delete[] sym_buffer;
    delete[] buffer_i16;
    delete[] buffer_i8;
    delete[] buffer_u8;
}

void FSKDemodModule::process()
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

    agcRun = quaRun = lpfRun = recRun = true;

    fileThread = std::thread(&FSKDemodModule::fileThreadFunction, this);
    agcThread = std::thread(&FSKDemodModule::agcThreadFunction, this);
    lpfThread = std::thread(&FSKDemodModule::lpfThreadFunction, this);
    quaThread = std::thread(&FSKDemodModule::quaThreadFunction, this);
    recThread = std::thread(&FSKDemodModule::clockrecoveryThreadFunction, this);

    int dat_size = 0;
    while (input_data_type == DATA_STREAM ? input_active.load() : !data_in.eof())
    {
        dat_size = rec_pipe.read();

        if (dat_size <= 0)
            continue;

        for (int i = 0; i < dat_size; i++)
        {
            sym_buffer[i] = clamp(rec_pipe.readBuf[i] * 50);
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

void FSKDemodModule::fileThreadFunction()
{
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

        in_pipe.swap(d_buffer_size); //->write(in_buffer, d_buffer_size);
    }

    if (input_data_type == DATA_FILE)
        data_in.close();

    // Exit all threads... Without causing a race condition!
    agcRun = quaRun = lpfRun = recRun = false;

    in_pipe.stopWriter();
    in_pipe.stopReader();

    agc_pipe.stopWriter();

    if (agcThread.joinable())
        agcThread.join();

    logger->debug("AGC OK");

    agc_pipe.stopReader();
    lpf_pipe.stopWriter();

    if (lpfThread.joinable())
        lpfThread.join();

    logger->debug("LPF OK");

    lpf_pipe.stopReader();
    qua_pipe.stopWriter();

    if (quaThread.joinable())
        quaThread.join();

    logger->debug("QUA OK");

    qua_pipe.stopReader();
    rec_pipe.stopWriter();

    if (recThread.joinable())
        recThread.join();

    logger->debug("REC OK");

    data_out.close();

    rec_pipe.stopReader();
}

void FSKDemodModule::agcThreadFunction()
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

void FSKDemodModule::lpfThreadFunction()
{
    int gotten;
    while (lpfRun)
    {
        gotten = agc_pipe.read(); //->read(agc_buffer2, d_buffer_size);

        if (gotten <= 0)
            continue;

        // Root-raised-cosine filtering
        int out = lpf->work(agc_pipe.readBuf, gotten, lpf_pipe.writeBuf);

        agc_pipe.flush();
        lpf_pipe.swap(out); //->write(rrc_buffer, out);
    }
}

void FSKDemodModule::quaThreadFunction()
{
    int gotten;
    while (quaRun)
    {
        gotten = lpf_pipe.read(); //->read(rrc_buffer2, d_buffer_size);

        if (gotten <= 0)
            continue;

        // Costas loop, frequency offset recovery
        qua->work(lpf_pipe.readBuf, gotten, qua_pipe.writeBuf);

        lpf_pipe.flush();
        qua_pipe.swap(gotten); //->write(pll_buffer, gotten);
    }
}

void FSKDemodModule::clockrecoveryThreadFunction()
{
    int gotten;
    while (recRun)
    {
        gotten = qua_pipe.read(); //->read(mov_buffer2, d_buffer_size);

        if (gotten <= 0)
            continue;

        int recovered_size = 0;

        try
        {
            // Clock recovery
            recovered_size = rec->work(qua_pipe.readBuf, gotten, rec_pipe.writeBuf);
        }
        catch (std::runtime_error &e)
        {
            logger->error(e.what());
        }

        qua_pipe.flush();
        rec_pipe.swap(recovered_size); //->write(rec_buffer, recovered_size);
    }
}

void FSKDemodModule::drawUI()
{
    ImGui::Begin("FSK Demodulator", NULL, NOWINDOW_FLAGS);

    // Constellation
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                 ImVec2(ImGui::GetCursorScreenPos().x + 200, ImGui::GetCursorScreenPos().y + 200),
                                 ImColor::HSV(0, 0, 0));

        for (int i = 0; i < 2048; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + rec_pipe.readBuf[i] * 45) % 200,
                                              ImGui::GetCursorScreenPos().y + (int)(100 + rng.gasdev() * 15) % 200),
                                       2,
                                       ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
        }

        ImGui::Dummy(ImVec2(200 + 3, 200 + 3));
    }

    ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

    ImGui::End();
}

std::string FSKDemodModule::getID()
{
    return "fsk_demod";
}

std::vector<std::string> FSKDemodModule::getParameters()
{
    return {"samplerate", "buffer_size", "baseband_format", "symbolrate", "lpf_cutoff", "lpf_transition_width"};
}

std::shared_ptr<ProcessingModule> FSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<FSKDemodModule>(input_file, output_file_hint, parameters);
}