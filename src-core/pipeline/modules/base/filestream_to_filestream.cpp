#include "filestream_to_filestream.h"
#include "logger.h"
#include "pipeline/module.h"

namespace satdump
{
    namespace pipeline
    {
        namespace base
        {
            FileStreamToFileStreamModule::FileStreamToFileStreamModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : ProcessingModule(input_file, output_file_hint, parameters)
            {
            }

            void FileStreamToFileStreamModule::init()
            {
                // Init input
                filesize = input_data_type == DATA_FILE ? getFilesize(d_input_file) : 0;
                if (input_data_type == DATA_FILE)
                {
                    data_in = std::ifstream(d_input_file, std::ios::binary);
                    logger->info("Using input file " + d_input_file);
                }
                progress = 0;

                if (fsfsm_enable_output)
                {
                    if (output_data_type == DATA_FILE)
                    {
                        data_out = std::ofstream(d_output_file_hint + fsfsm_file_ext, std::ios::binary);
                        logger->info("Ouputing to " + d_output_file_hint + fsfsm_file_ext);
                    }

                    d_output_file = d_output_file_hint + fsfsm_file_ext;
                }
            }

            nlohmann::json FileStreamToFileStreamModule::getModuleStats()
            {
                nlohmann::json v;
                if (!d_is_streaming_input)
                    v["progress"] = ((double)progress / (double)filesize) * 100.0f;
                return v;
            }

            void FileStreamToFileStreamModule::read_data(uint8_t *buffer, size_t size)
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                {
                    data_in.read((char *)buffer, size);
                    progress = data_in.tellg();
                }
                else
                {
                    input_fifo->read((uint8_t *)buffer, size);
                }

                if (time(0) - time_)
                {
                    logger->trace(getModuleStats().dump(4));
                    time_ = time(0);
                }
            }

            void FileStreamToFileStreamModule::write_data(uint8_t *buffer, size_t size)
            {
                // Write buffer
                if (output_data_type == DATA_FILE)
                    data_out.write((char *)buffer, size);
                else
                    output_fifo->write((uint8_t *)buffer, size);
            }

            bool FileStreamToFileStreamModule::should_run() { return input_data_type == DATA_FILE ? !data_in.eof() : input_active.load(); }

            void FileStreamToFileStreamModule::cleanup()
            {
                if (input_data_type == DATA_FILE)
                    data_in.close();
                if (output_data_type == DATA_FILE)
                    data_out.close();
            }

            void FileStreamToFileStreamModule::drawProgressBar()
            {
                if (!d_is_streaming_input)
                    ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));
            }
        } // namespace base
    } // namespace pipeline
} // namespace satdump