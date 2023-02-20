#pragma once

#include "core/module.h"
#include "common/codings/generic_correlator.h"
#include "common/codings/viterbi/viterbi27.h"
#include "decode_utils.h"
#include <fstream>

namespace inmarsat
{
    namespace aero
    {
        class AeroDecoderModule : public ProcessingModule
        {
        protected:
            bool is_c_channel = false;

            bool d_aero_oqpsk;
            int d_aero_dummy_bits;
            int d_aero_interleaver_cols;
            int d_aero_interleaver_blocks;

            int d_aero_sync_size;
            int d_aero_hdr_size;
            int d_aero_interleaver_block_size;
            int d_aero_info_size;
            int d_aero_total_frm_size;

            float d_aero_ber_thresold = 1.0;

            int8_t *soft_buffer;
            int8_t *buffer_deinterleaved;
            uint8_t *buffer_vitdecoded;

            std::ifstream data_in;
            std::ofstream data_out;
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            std::unique_ptr<CorrelatorGeneric> correlator;
            std::unique_ptr<viterbi::Viterbi27> viterbi;
            std::vector<uint8_t> randomization_seq;

            // UI Stuff
            float ber_history[200];
            float cor_history[200];

            float correlator_cor = 0;
            bool correlator_locked = false;

        public:
            AeroDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~AeroDecoderModule();
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