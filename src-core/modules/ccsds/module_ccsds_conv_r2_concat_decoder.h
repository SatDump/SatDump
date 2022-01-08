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
    This decoder is meant to decode convolutional r=2 k=7 codes
    concatenated with Reed-Solomon parity bits. This soft of FEC
    is pretty common on CCSDS-compliant satellites, with some
    varients such as :
        - Differential (NRZ-M) encoding
        - Different CADU size
        - RS 223 or 239 codes, with usually I=4 or I=5
        - Bit swap and 90 degs phase rotation on BPSK
        - Reed-Solomon lacking dual-basis

    All those variations are in the end pretty minor so a common
    decoder can be used instead allowing a high degree of tuning.

    Decoding is done by first locking a streaming viterbi decoder
    onto a specific state of the provided modulation, to then feed
    the decoded data to a deframer.

    It is recommended to use a rather low thresold for the Viterbi
    decoder, usually just below the average to ensure it locks as
    soon as possible. 0.300 seems to be good.

    The ASM Marker is left configurable as other satellites use 
    similar protocols, just with a different syncword. 

    CCSDS naming is kept mostly because this specific convolutional
    code is from the specification and most satellites will use
    CCSDS-compliant concatenated codings anyway.
    */
    class CCSDSConvR2ConcatDecoderModule : public ProcessingModule
    {
    protected:
        const bool is_ccsds; // Just to know if we should output .cadu or .frm

        const std::string d_constellation_str;     // Constellation type string
        dsp::constellation_type_t d_constellation; // Constellation type
        bool d_bpsk_90;                            // Special case for BPSK shifted by 90 degs + IQ-swapped

        const int d_cadu_size;   // CADU Size in bits, including ASM
        const int d_cadu_bytes;  // CADU Size in bytes, including ASM
        const int d_buffer_size; // Processing buffer size, default half of a frame (= d_cadu_size)

        const bool d_diff_decode; // If NRZ-M Decoding is required or not

        const bool d_derand;          // Perform derandomizion or not
        const bool d_derand_after_rs; // Derandomization after RS
        const int d_derand_from;      // Byte to start derand on

        const int d_rs_interleaving_depth; // RS Interleaving depth. If = 0, then RS is disabled
        const bool d_rs_dualbasis;         // RS Representation. Dual basis or none?
        const std::string d_rs_type;       // RS Type identifier

        const int d_viterbi_outsync_after;
        const float d_viterbi_ber_threasold;

        uint8_t *viterbi_out;
        int8_t *soft_buffer;
        uint8_t *frame_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        std::shared_ptr<viterbi::Viterbi1_2> viterbi;
        std::shared_ptr<deframing::BPSK_CCSDS_Deframer> deframer;
        std::shared_ptr<reedsolomon::ReedSolomon> reed_solomon;

        int errors[10];

        // UI Stuff
        float ber_history[200];
        dsp::Random rng;

    public:
        CCSDSConvR2ConcatDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~CCSDSConvR2ConcatDecoderModule();
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