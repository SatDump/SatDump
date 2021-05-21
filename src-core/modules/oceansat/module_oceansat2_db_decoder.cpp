#include "module_oceansat2_db_decoder.h"
#include "logger.h"
#include "common/differential/generic.h"
#include "imgui/imgui.h"
#include "oceansat2_deframer.h"
#include "oceansat2_derand.h"

#define BUFFER_SIZE 8192

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace oceansat
{
    // Modified to only repack the OCM channel
    class RepackOneChannel
    {
    private:
        uint8_t byteToWrite = 0;
        int inByteToWrite = 0;

    public:
        int work(uint8_t *in, int length, uint8_t *out)
        {
            int oo = 0;

            for (int ii = 0; ii < length; ii++)
            {
                byteToWrite = byteToWrite << 1 | ((in[ii] >> 1) & 1);
                inByteToWrite++;

                if (inByteToWrite == 8)
                {
                    out[oo++] = byteToWrite;
                    inByteToWrite = 0;
                }
            }

            return oo;
        }
    };

    Oceansat2DBDecoderModule::Oceansat2DBDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new int8_t[BUFFER_SIZE];
        frame_count = 0;
    }

    Oceansat2DBDecoderModule::~Oceansat2DBDecoderModule()
    {
        delete[] buffer;
    }

    void Oceansat2DBDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".ocm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".ocm");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".ocm");

        time_t lastTime = 0;

        // I/Q Buffers
        uint8_t decodedBuffer1[BUFFER_SIZE], decodedBuffer2[BUFFER_SIZE];
        uint8_t diffedBuffer1[BUFFER_SIZE], diffedBuffer2[BUFFER_SIZE];

        // Final buffer after decoding
        uint8_t finalBuffer1[BUFFER_SIZE], finalBuffer2[BUFFER_SIZE];

        // Bits => Bytes stuff
        RepackOneChannel repack1, repack2;

        diff::GenericDiff diffdecoder1(4), diffdecoder2(4);

        Oceansat2Deframer deframer1, deframer2;
        Oceansat2Derand derand;

        // In this decoder both swapped and normal I/Q are always decoded. This does is crappier
        // than correlation, but works well enough and faster. And anyway, we aren't dealing with
        // that much data.
        // We can also safely assume frames will NEVER be found on both states in some weird way...
        // So I guess this is fine enough?

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, BUFFER_SIZE);

            // Demodulate DQPSK... This is the crappy way but it works
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                // Normal IQ
                bool a = buffer[i * 2 + 0] > 0;
                bool b = buffer[i * 2 + 1] > 0;

                if (a)
                {
                    if (b)
                        decodedBuffer1[i] = 0x0;
                    else
                        decodedBuffer1[i] = 0x3;
                }
                else
                {
                    if (b)
                        decodedBuffer1[i] = 0x1;
                    else
                        decodedBuffer1[i] = 0x2;
                }

                // Inversed IQ
                a = buffer[i * 2 + 1] > 0;
                b = buffer[i * 2 + 0] > 0;

                if (a)
                {
                    if (b)
                        decodedBuffer2[i] = 0x0;
                    else
                        decodedBuffer2[i] = 0x3;
                }
                else
                {
                    if (b)
                        decodedBuffer2[i] = 0x1;
                    else
                        decodedBuffer2[i] = 0x2;
                }
            }

            int diff_count1 = diffdecoder1.work(decodedBuffer1, BUFFER_SIZE / 2, diffedBuffer1); // Diff normal IQ
            int diff_count2 = diffdecoder2.work(decodedBuffer2, BUFFER_SIZE / 2, diffedBuffer2); // Diff inversed IQ

            int byteCount1 = repack1.work(diffedBuffer1, diff_count1, finalBuffer1); // Repack normal IQ
            int byteCount2 = repack2.work(diffedBuffer2, diff_count2, finalBuffer2); // Repack inversed IQ

            std::vector<std::vector<uint8_t>> frames1 = deframer1.work(finalBuffer1, byteCount1); // Deframe normal IQ
            std::vector<std::vector<uint8_t>> frames2 = deframer2.work(finalBuffer2, byteCount2); // Deframe inversed IQ

            frame_count += frames1.size() + frames2.size();

            // Process normal IQ
            for (std::vector<uint8_t> frame : frames1)
            {
                derand.work(frame.data());
                data_out.write((char *)frame.data(), 92220);
            }

            // Process inversed IQ
            for (std::vector<uint8_t> frame : frames2)
            {
                derand.work(frame.data());
                data_out.write((char *)frame.data(), 92220);
            }

            // This above, just processing without checking what frame came first is only fine because we are using a smaller
            // buffer size than frame size. First will get out first.

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Frames : " + std::to_string(frame_count));
            }
        }

        data_out.close();
        data_in.close();
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void Oceansat2DBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Oceansat-2 DB Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string Oceansat2DBDecoderModule::getID()
    {
        return "oceansat2_db_decoder";
    }

    std::vector<std::string> Oceansat2DBDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Oceansat2DBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<Oceansat2DBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace aqua