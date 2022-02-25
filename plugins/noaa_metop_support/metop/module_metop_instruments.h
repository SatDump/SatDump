#pragma once

#include "module.h"
#include "common/widgets/image_preview.h"

#include "instruments/avhrr/avhrr_reader.h"
#include "instruments/mhs/mhs_reader.h"
#include "instruments/ascat/ascat_reader.h"
#include "instruments/iasi/iasi_reader.h"
#include "instruments/iasi/iasi_imaging_reader.h"
#include "instruments/amsu/amsu_a1_reader.h"
#include "instruments/amsu/amsu_a2_reader.h"
#include "instruments/gome/gome_reader.h"
#include "instruments/sem/sem_reader.h"
#include "instruments/admin_msg/admin_msg_reader.h"

namespace metop
{
    namespace instruments
    {
        class MetOpInstrumentsDModule : public ProcessingModule
        {
        protected:
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            // int shown_avhrr_channel = 3;
            // int shown_mhs_channel = 2;
            // ImagePreviewWidget avhrr_image_preview;
            // ImagePreviewWidget mhs_image_preview;
            // ImagePreviewWidget iasi_image_preview;

            // Readers
            avhrr::AVHRRReader avhrr_reader;
            mhs::MHSReader mhs_reader;
            ascat::ASCATReader ascat_reader;
            iasi::IASIReader iasi_reader;
            iasi::IASIIMGReader iasi_reader_img;
            amsu::AMSUA1Reader amsu_a1_reader;
            amsu::AMSUA2Reader amsu_a2_reader;
            gome::GOMEReader gome_reader;
            sem::SEMReader sem_reader;
            admin_msg::AdminMsgReader admin_msg_reader;

        public:
            MetOpInstrumentsDModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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