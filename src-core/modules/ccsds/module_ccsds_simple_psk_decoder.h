#pragma once

#include "module.h"
#include <complex>
#include "common/codings/viterbi/viterbi_1_2.h"
#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include <fstream>
#include "common/dsp/constellation.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/dsp/random.h"

namespace ccsds
{
    /*
    This decoder is meant to decode simple PSK signals
    using or akin to the CCSDS standard.

    Frame length, differential encoding or not are kept
    configurable, alongside the ASM as this same decoder
    can cover a lot of other specifications.
    */
    class CCSDSSimplePSKDecoderModule : public ProcessingModule
    {
    protected:
        const bool is_ccsds; // Just to know if we should output .cadu or .frm

        const std::string d_constellation_str;     // Constellation type string
        dsp::constellation_type_t d_constellation; // Constellation type

        const int d_cadu_size;   // CADU Size in bits, including ASM
        const int d_cadu_bytes;  // CADU Size in bytes, including ASM
        const int d_buffer_size; // Processing buffer size, default half of a frame (= d_cadu_size)

        const bool d_qpsk_swapiq;   // If IQ should be swapped before being demodulated (to support different DQPSK modes)
        const bool d_qpsk_swapdiff; // If bits should be swapped after the output of the QPSK diff decoder

        const bool d_diff_decode; // If NRZ-M Decoding is required or not

        const bool d_derand;          // Perform derandomizion or not
        const bool d_derand_after_rs; // Derandomization after RS
        const int d_derand_from;      // Byte to start derand on

        const int d_rs_interleaving_depth; // RS Interleaving depth. If = 0, then RS is disabled
        const bool d_rs_dualbasis;         // RS Representation. Dual basis or none?
        const std::string d_rs_type;       // RS Type identifier

        uint8_t *bits_out;
        int8_t *soft_buffer;
        uint8_t *qpsk_diff_buffer;
        uint8_t *frame_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        std::shared_ptr<deframing::BPSK_CCSDS_Deframer> deframer;
        std::shared_ptr<reedsolomon::ReedSolomon> reed_solomon;

        int errors[10];

        // UI Stuff
        dsp::Random rng;

    public:
        CCSDSSimplePSKDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~CCSDSSimplePSKDecoderModule();
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
} // namespace npp