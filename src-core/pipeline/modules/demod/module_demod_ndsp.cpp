#include "module_demod_ndsp.h"
#include "imgui/imgui.h"
#include "pipeline/module.h"
#include <chrono>
#include <thread>

namespace satdump
{
    namespace pipeline
    {
        namespace demod
        {
            NdspDemodModule::NdspDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters) {}

            void NdspDemodModule::init()
            {
                old2new_block = std::make_shared<ndsp::Old2NewBlock<complex_t>>();
                demod_block = std::make_shared<ndsp::PSKDemodHierBlock>();
                const_block = std::make_shared<ndsp::ConstellationDisplayBlock>();

                old2new_block->old_stream_exit = false;
                old2new_block->old_stream = input_stream;

                demod_block->set_cfg("constellation", "qpsk");
                demod_block->set_cfg("samplerate", 6e6);
                demod_block->set_cfg("symbolrate", 2.33e6);

                demod_block->link(old2new_block.get(), 0, 0, 4);
                const_block->link(demod_block.get(), 0, 0, 4);
            }

            std::vector<ModuleDataType> NdspDemodModule::getInputTypes() { return {DATA_FILE, DATA_DSP_STREAM}; }

            std::vector<ModuleDataType> NdspDemodModule::getOutputTypes() { return {DATA_FILE, DATA_STREAM}; }

            NdspDemodModule::~NdspDemodModule() {}

            void NdspDemodModule::process()
            {
                old2new_block->start();
                demod_block->start();
                const_block->start();

                while (!old2new_block->old_stream_exit)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));

                old2new_block->stop();
                demod_block->stop(false, false);
                const_block->stop();
            }

            void NdspDemodModule::stop()
            {
                old2new_block->old_stream_exit = true;
                old2new_block->stop(true);
                demod_block->stop(true, false);
                const_block->stop(true);
            }

            void NdspDemodModule::drawUI(bool window)
            {
                ImGui::Begin("NDSP Demod", NULL, window ? 0 : NOWINDOW_FLAGS);

                const_block->constel.draw();

                // if (!d_is_streaming_input)
                //     ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

                ImGui::End();
            }

            std::shared_ptr<ProcessingModule> NdspDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            {
                return std::make_shared<NdspDemodModule>(input_file, output_file_hint, parameters);
            }
        } // namespace demod
    } // namespace pipeline
} // namespace satdump