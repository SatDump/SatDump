#pragma once

#include "core/module.h"
#include "instruments/atms/atms_reader.h"
#include "instruments/omps/omps_nadir_reader.h"
#include "instruments/omps/omps_limb_reader.h"
#include "instruments/viirs/channel_reader.h"
#include "instruments/att_ephem/att_ephem_reader.h"

namespace jpss
{
    namespace instruments
    {
        class JPSSInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            const bool npp_mode;

            // Readers
            atms::ATMSReader atms_reader;
            omps::OMPSNadirReader omps_nadir_reader;
            omps::OMPSNadirReader omps_limb_reader;

            att_ephem::AttEphemReader att_ephem;

            // Readers for all VIIRS APIDs, in order!
            viirs::VIIRSReader viirs_reader_moderate[16] = {viirs::VIIRSChannels[804], viirs::VIIRSChannels[803], viirs::VIIRSChannels[802], viirs::VIIRSChannels[800],
                                                            viirs::VIIRSChannels[801], viirs::VIIRSChannels[805], viirs::VIIRSChannels[806], viirs::VIIRSChannels[809],
                                                            viirs::VIIRSChannels[807], viirs::VIIRSChannels[808], viirs::VIIRSChannels[810], viirs::VIIRSChannels[812],
                                                            viirs::VIIRSChannels[811], viirs::VIIRSChannels[816], viirs::VIIRSChannels[815], viirs::VIIRSChannels[814]};
            viirs::VIIRSReader viirs_reader_imaging[5] = {viirs::VIIRSChannels[818], viirs::VIIRSChannels[819], viirs::VIIRSChannels[820],
                                                          viirs::VIIRSChannels[813], viirs::VIIRSChannels[817]};
            viirs::VIIRSReader viirs_reader_dnb[3] = {viirs::VIIRSChannels[821], viirs::VIIRSChannels[822],
                                                      viirs::VIIRSChannels[823]};

            // Statuses
            instrument_status_t atms_status = DECODING;
            instrument_status_t omps_nadir_status = DECODING;
            instrument_status_t omps_limb_status = DECODING;
            instrument_status_t viirs_moderate_status[16] = {DECODING, DECODING, DECODING, DECODING,
                                                             DECODING, DECODING, DECODING, DECODING,
                                                             DECODING, DECODING, DECODING, DECODING,
                                                             DECODING, DECODING, DECODING, DECODING};
            instrument_status_t viirs_imaging_status[5] = {DECODING, DECODING, DECODING, DECODING, DECODING};
            instrument_status_t viirs_dnb_status = DECODING;

            // VIIRS Stuff
            void process_viirs_channels();

        public:
            JPSSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace amsu
} // namespace metop