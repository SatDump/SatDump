#include "module_uvsqsatng_instruments.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "img_payload.h"

namespace uvsq
{
    UVSQSatNGInstrumentsDecoderModule::UVSQSatNGInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    void UVSQSatNGInstrumentsDecoderModule::process()
    {
        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        uint8_t cadu[259];

        std::string jpeg_dir = directory + "/JPEG";
        if (!std::filesystem::exists(jpeg_dir))
            std::filesystem::create_directories(jpeg_dir);
        ImgPayloadDecoder jpeg_dec(0, jpeg_dir);

        std::string raw_dir = directory + "/RAW";
        if (!std::filesystem::exists(raw_dir))
            std::filesystem::create_directories(raw_dir);
        ImgPayloadDecoder raw_dec(1, raw_dir);

        std::ofstream data_ou(directory + "/others.cadu", std::ios::binary);

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)cadu, 259);

            // Not sure yet what this one is
            //            int mkr1 = cadu[5] & 0xF;

            // Channel/Payload marker
            int channel = cadu[15];

            if (channel == 202) // JPEG Camera images
            {
                jpeg_dec.work(cadu);
            }
            else if (channel == 210)
            {
                raw_dec.work(cadu);
            }
            else
            {
                data_ou.write((char *)cadu, 259);
            }
        }

        cleanup();

        jpeg_dec.save();
        raw_dec.save();
    }

    void UVSQSatNGInstrumentsDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("UVSQSat-NG Instruments", NULL, window ? 0 : NOWINDOW_FLAGS);

        drawProgressBar();

        ImGui::End();
    }

    std::string UVSQSatNGInstrumentsDecoderModule::getID() { return "uvsqsatng_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> UVSQSatNGInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<UVSQSatNGInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace uvsq