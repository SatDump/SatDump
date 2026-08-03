#include "module_ggak_instruments.h"
#include "ggak.h"
#include "imgui/imgui.h"
#include "skl.h"
#include <cstdint>
#include <cstdio>

namespace elektro_arktika
{
    namespace ggak
    {
        GGAKInstrumentsDecoderModule::GGAKInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = true;
            fsfsm_file_ext = ".cadu";

            is_arktika = parameters.contains("is_arktika") ? parameters["is_arktika"].get<bool>() : false;
            if (parameters.contains("satellite_number"))
                sat_num = parameters["satellite_number"].is_string() ? std::stoi(parameters["satellite_number"].get<std::string>()) : parameters["satellite_number"].get<int>();
            else
                sat_num = 0;
        }

        void GGAKInstrumentsDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

            GGAKFrame frm;

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&frm, 224);

                records_mtx.lock();

                if (frm.id == 48)
                {
                    auto recs = parseSKLRecord(&frm);
                    skl_records.insert(skl_records.end(), recs.begin(), recs.end());
                }

                records_mtx.unlock();

                // Write to save cadu
                write_data((uint8_t *)&frm, 224);
            }
        }

        void GGAKInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("ELEKTRO / ARKTIKA GGAK Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string GGAKInstrumentsDecoderModule::getID() { return "elektro_arktika_ggak_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GGAKInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GGAKInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ggak
} // namespace elektro_arktika