#include "module_soft2hard.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>

#include "common/codings/differential/nrzm.h"

namespace satdump
{
    namespace pipeline
    {
        namespace generic
        {
            Soft2HardModule::Soft2HardModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
            {
                input_buffer = new int8_t[256];
            }

            Soft2HardModule::~Soft2HardModule() { delete[] input_buffer; }

            void Soft2HardModule::process()
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

                uint8_t byte_shifter = 0;
                int bitshift_in_byte = 0;
                uint8_t byte_buffer[256];
                int bytes_in_buf = 0;

                diff::NRZMDiff diff;

                while (should_run())
                {
                    // Read buffer
                    read_data((uint8_t *)input_buffer, 256);

                    for (int i = 0; i < 256; i++)
                    {
                        // Slice
                        uint8_t bit = input_buffer[i] > 0;

                        byte_shifter = byte_shifter << 1 | bit;
                        bitshift_in_byte++;

                        if (bitshift_in_byte == 8)
                        {
                            byte_buffer[bytes_in_buf] = byte_shifter;
                            bitshift_in_byte = 0;
                            bytes_in_buf++;

                            if (bytes_in_buf == 256)
                            {
                                diff.decode(byte_buffer, 256);

                                write_data(byte_buffer, 256);

                                bytes_in_buf = 0;
                            }
                        }
                    }
                }

                cleanup();
            }

            void Soft2HardModule::drawUI(bool window)
            {
                ImGui::Begin("Soft To Hard", NULL, window ? 0 : NOWINDOW_FLAGS);

                drawProgressBar();

                ImGui::End();
            }

            std::string Soft2HardModule::getID() { return "soft2hard"; }

            std::shared_ptr<ProcessingModule> Soft2HardModule::getInstance(nlohmann::json parameters, std::string input_file, std::string output_file_hint)
            {
                return std::make_shared<Soft2HardModule>(input_file, output_file_hint, parameters);
            }
        } // namespace generic
    } // namespace pipeline
} // namespace satdump