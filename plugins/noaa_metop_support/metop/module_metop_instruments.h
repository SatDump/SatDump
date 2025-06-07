#pragma once

#include "../instruments/amsu/amsu_reader.h"
#include "../instruments/avhrr/avhrr_reader.h"
#include "../instruments/mhs/mhs_reader.h"
#include "instruments/admin_msg/admin_msg_reader.h"
#include "instruments/ascat/ascat_reader.h"
#include "instruments/avhrr/avhrr_to_hpt.h"
#include "instruments/gome/gome_reader.h"
#include "instruments/iasi/iasi_imaging_reader.h"
#include "instruments/iasi/iasi_reader.h"
#include "instruments/sem/sem_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace metop
{
    namespace instruments
    {
        class MetOpInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            bool write_hpt = false;

            bool ignore_integrated_tle = false;

            // Readers
            noaa_metop::avhrr::AVHRRReader avhrr_reader;
            noaa_metop::mhs::MHSReader mhs_reader;
            ascat::ASCATReader ascat_reader;
            iasi::IASIReader iasi_reader;
            iasi::IASIIMGReader iasi_reader_img;
            noaa_metop::amsu::AMSUReader amsu_reader;
            gome::GOMEReader gome_reader;
            sem::SEMReader sem_reader;
            admin_msg::AdminMsgReader admin_msg_reader;
            avhrr::AVHRRToHpt *avhrr_to_hpt;

            // Statuses
            instrument_status_t avhrr_status = DECODING;
            instrument_status_t iasi_status = DECODING;
            instrument_status_t iasi_img_status = DECODING;
            instrument_status_t mhs_status = DECODING;
            instrument_status_t amsu_status = DECODING;
            instrument_status_t gome_status = DECODING;
            instrument_status_t ascat_status = DECODING;
            instrument_status_t sem_status = DECODING;
            instrument_status_t admin_msg_status = DECODING;

        public:
            MetOpInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams()
            {
                nlohmann::json v; // TODOREWORk
                v["write_hpt"] = false;
                v["ignore_integrated_tle"] = false;
                return v;
            }
            static std::shared_ptr<satdump::pipeline::ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace metop