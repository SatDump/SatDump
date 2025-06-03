#pragma once

#include "instruments/amr2/amr2_reader.h"
#include "instruments/lpt/lpt_reader.h"
#include "instruments/poseidon/poseidon_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace jason3
{
    namespace instruments
    {
        class Jason3InstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            // Readers
            amr2::AMR2Reader amr2_reader;
            poseidon::PoseidonReader poseidon_c_reader;
            poseidon::PoseidonReader poseidon_ku_reader;
            lpt::LPTReader lpt_els_a_reader = lpt::LPTReader(16 - 6, 22, 64);
            lpt::LPTReader lpt_els_b_reader = lpt::LPTReader(18 - 6, 13, 50);
            lpt::LPTReader lpt_aps_a_reader = lpt::LPTReader(18 - 6, 49, 120);
            lpt::LPTReader lpt_aps_b_reader = lpt::LPTReader(18 - 6, 38, 98);

            // Statuses
            instrument_status_t amr2_status = DECODING;
            instrument_status_t poseidon_c_status = DECODING;
            instrument_status_t poseidon_ku_status = DECODING;
            instrument_status_t lpt_els_a_status = DECODING;
            instrument_status_t lpt_els_b_status = DECODING;
            instrument_status_t lpt_aps_a_status = DECODING;
            instrument_status_t lpt_aps_b_status = DECODING;

        public:
            Jason3InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace jason3