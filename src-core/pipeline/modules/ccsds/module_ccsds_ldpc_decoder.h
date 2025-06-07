#pragma once

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/codings/generic_correlator.h"
#include "common/codings/ldpc/ccsds_ldpc.h"
#include "common/dsp/demod/constellation.h"
#include "common/dsp/utils/random.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace satdump
{
    namespace pipeline
    {
        namespace ccsds
        {
            /*
            This decoder is meant to decode LDPC codes from the
            CCSDS 131.0 Specification. Everything from the 7/8
            code to the AR4JA 1/2, 2/3, 3/4 codes should be covered.
            Optionally, the actual CADUs can be in a stream of STMFs.
            */
            class CCSDSLDPCDecoderModule : public base::FileStreamToFileStreamModule
            {
            protected:
                const bool is_ccsds; // Just to know if we should output .cadu or .frm

                const std::string d_constellation_str;     // Constellation type string
                dsp::constellation_type_t d_constellation; // Constellation type
                // const bool d_iq_invert;                    // For some QPSK sats, can need to be inverted...

                const bool d_derand; // Perform derandomizion or not

                std::string d_ldpc_rate_str;            // LDPC Rate string
                codings::ldpc::ldpc_rate_t d_ldpc_rate; // LDPC Rate
                int d_ldpc_block_size;                  // LDPC Block size (for AR4JA only)
                int d_ldpc_iterations;                  // LDPC Iterations

                const bool d_internal_stream; // Does this have an internal CADU stream?
                const int d_cadu_size;        // CADU Size in bits, including ASM
                const int d_cadu_bytes;       // CADU Size in bytes, including ASM

                int d_ldpc_frame_size;
                int d_ldpc_codeword_size;
                int d_ldpc_asm_size;
                int d_ldpc_simd;
                int d_ldpc_data_size;

                int8_t *soft_buffer;
                int frames_in_ldpc_buffer;
                int8_t *ldpc_input_buffer;
                uint8_t *ldpc_output_buffer;
                uint8_t *deframer_buffer;

                std::unique_ptr<CorrelatorGeneric> correlator;
                std::unique_ptr<codings::ldpc::CCSDSLDPC> ldpc_dec;
                std::unique_ptr<deframing::BPSK_CCSDS_Deframer> deframer;

                // UI Stuff
                float ber_history[200];
                dsp::Random rng;

                // UI Stuff
                float cor_history[200];

                float correlator_cor;
                bool correlator_locked;

                float ldpc_history[200];
                int ldpc_corr;

                bool is_started = false;

            public:
                CCSDSLDPCDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~CCSDSLDPCDecoderModule();
                void process();
                void drawUI(bool window);

                nlohmann::json getModuleStats();

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); };
                static nlohmann::json getParams() { return {}; } // TODOREWORK
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace ccsds
    } // namespace pipeline
} // namespace satdump