#pragma once

#include "core/module.h"

#include "instruments/xeuvi/xeuvi_reader.h"
#include "instruments/windrad/windrad_reader.h"
#include "instruments/mwhs2/mwhs2_reader.h"
#include "instruments/mwts/mwts_reader.h"
#include "instruments/mwts2/mwts2_reader.h"
#include "instruments/mwts3/mwts3_reader.h"
#include "instruments/mwri/mwri_reader.h"
#include "instruments/mwri2/mwri2_reader.h"
#include "instruments/mwrirm/mwrirm_reader.h"
#include "instruments/wai/wai_reader.h"
#include "instruments/virr/virr_reader.h"
#include "instruments/mwhs/mwhs_reader.h"
#include "instruments/erm/erm_reader.h"
#include "instruments/mersi/mersi_reader.h"
#include "instruments/gas/gas_reader.h"
#include "instruments/virr/virr_to_c10.h"
#include "instruments/pmr/pmr_reader.h"

namespace fengyun3
{
    namespace instruments
    {
        class FY3InstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            enum fy3_sat_t
            {
                FY_AB,
                FY_3C,
                FY_3D,
                FY_3E,
                FY_3F,
                FY_3G,
            };

            enum fy3_downlink_t
            {
                AHRPT,
                MPT,
                DPT,
            };

            bool is_init = false;

            fy3_sat_t d_satellite;
            fy3_downlink_t d_downlink;
            bool d_mersi_bowtie;
            bool d_mersi_histmatch;
            bool d_dump_mersi;
            bool d_write_c10;

            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            erm::ERMReader erm_reader;
            virr::VIRRReader virr_reader;
            std::unique_ptr<wai::WAIReader> wai_reader;
            mwri::MWRIReader mwri_reader;
            mwri2::MWRI2Reader mwri2_reader;
            mwrirm::MWRIRMReader mwrirm_reader;
            mwts::MWTSReader mwts1_reader;
            mwts2::MWTS2Reader mwts2_reader;
            mwts3::MWTS3Reader mwts3_reader;
            mwhs::MWHSReader mwhs1_reader;
            mwhs2::MWHS2Reader mwhs2_reader;
            std::unique_ptr<windrad::WindRADReader> windrad_reader1, windrad_reader2;
            std::unique_ptr<xeuvi::XEUVIReader> xeuvi_reader;
            std::unique_ptr<pmr::PMRReader> pmr1_reader;
            std::unique_ptr<pmr::PMRReader> pmr2_reader;
            mersi::MERSI1Reader mersi1_reader;
            mersi::MERSI2Reader mersi2_reader;
            mersi::MERSI3Reader mersi3_reader;
            mersi::MERSILLReader mersill_reader;
            mersi::MERSIRMReader mersirm_reader;
            gas::GASReader gas_reader;
            virr::VIRRToC10 *virr_to_c10;

            // Statuses
            instrument_status_t mersi1_status = DECODING;
            instrument_status_t mersi2_status = DECODING;
            instrument_status_t mersi3_status = DECODING;
            instrument_status_t mersill_status = DECODING;
            instrument_status_t mersirm_status = DECODING;
            instrument_status_t erm_status = DECODING;
            instrument_status_t virr_status = DECODING;
            instrument_status_t wai_status = DECODING;
            instrument_status_t gas_status = DECODING;
            instrument_status_t mwri_status = DECODING;
            instrument_status_t mwri2_status = DECODING;
            instrument_status_t mwrirm_status = DECODING;
            instrument_status_t mwts1_status = DECODING;
            instrument_status_t mwts2_status = DECODING;
            instrument_status_t mwts3_status = DECODING;
            instrument_status_t mwhs1_status = DECODING;
            instrument_status_t mwhs2_status = DECODING;
            instrument_status_t xeuvi_status = DECODING;
            instrument_status_t pmr1_status = DECODING;
            instrument_status_t pmr2_status = DECODING;
            instrument_status_t windrad_C_status = DECODING;
            instrument_status_t windrad_Ku_status = DECODING;

        public:
            FY3InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace modis
} // namespace eos