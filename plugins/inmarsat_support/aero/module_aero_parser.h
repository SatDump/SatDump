#pragma once

#include "core/module.h"
#include "decode_utils.h"
#include <fstream>
#include "common/net/udp.h"
#include "pkt_structs.h"
#include "acars_parser.h"

#ifdef ENABLE_AUDIO
#include "rtaudio/RtAudio.h"
#endif

namespace inmarsat
{
    namespace aero
    {
        class AeroParserModule : public ProcessingModule
        {
        protected:
            uint8_t *buffer;

            bool is_c_channel = false;

            std::ifstream data_in;
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            std::mutex pkt_history_mtx;
            std::vector<nlohmann::json> pkt_history;
            std::vector<nlohmann::json> pkt_history_acars;

            void process_final_pkt(nlohmann::json &msg);

            bool do_save_files;
            std::vector<std::shared_ptr<net::UDPClient>> udp_clients;

            bool is_gui = false;

            bool has_wip_user_data = false;
            pkts::MessageUserDataFinal wip_user_data;
            acars::ACARSParser acars_parser;
            void process_pkt();

#ifdef ENABLE_AUDIO
            std::mutex audio_mtx;
            std::vector<int16_t> audio_buff;

            RtAudio rt_dac;
            RtAudio::StreamParameters rt_parameters;

            static int audio_callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
            {
                AeroParserModule *tthis = (AeroParserModule *)userData;
                int16_t *buffer = (int16_t *)outputBuffer;

                tthis->audio_mtx.lock();
                int in_vec = tthis->audio_buff.size();
                tthis->audio_mtx.unlock();

                if (in_vec > nBufferFrames)
                {
                    tthis->audio_mtx.lock();
                    memcpy(buffer, tthis->audio_buff.data(), nBufferFrames * sizeof(int16_t));
                    tthis->audio_buff.erase(tthis->audio_buff.begin(), tthis->audio_buff.begin() + nBufferFrames);
                    tthis->audio_mtx.unlock();
                }
                else
                {
                    memset(buffer, 0, nBufferFrames * sizeof(int16_t));
                }

                return 0;
            }
#endif

        public:
            AeroParserModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~AeroParserModule();
            void process();
            void drawUI(bool window);
            std::vector<ModuleDataType> getInputTypes();
            std::vector<ModuleDataType> getOutputTypes();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    }
}