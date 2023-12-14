#pragma once

#include "core/module.h"
#include "common/repack.h"
#include "../instruments/avhrr/avhrr_reader.h"
#include "../instruments/mhs/mhs_reader.h"
#include "instruments/hirs/hirs_reader.h"
#include "../instruments/amsu/amsu_reader.h"
#include "instruments/sem/sem_reader.h"
#include "instruments/telemetry/telemetry_reader.h"

namespace noaa
{
    namespace instruments
    {
        class NOAAInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;
            const bool is_gac;
            const bool is_dsb;

            // Readers
            noaa_metop::avhrr::AVHRRReader avhrr_reader;
            noaa_metop::mhs::MHSReader mhs_reader;
            hirs::HIRSReader hirs_reader;
            noaa_metop::amsu::AMSUReader amsu_reader;
            sem::SEMReader sem_reader;
            telemetry::TelemetryReader telemetry_reader;

            // Statuses
            instrument_status_t avhrr_status = DECODING;
            instrument_status_t mhs_status = DECODING;
            instrument_status_t amsu_status = DECODING;
            instrument_status_t hirs_status = DECODING;
            instrument_status_t sem_status = DECODING;
            instrument_status_t telemetry_status = DECODING;

        public:
            NOAAInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace amsu
} // namespace noaa
