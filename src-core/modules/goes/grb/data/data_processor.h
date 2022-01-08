#pragma once

#include "common/image/image.h"
#include <memory>
#include "abi/abi_image_assembler.h"
#include "suvi/suvi_image_assembler.h"

/*
All the information used to fill in products APIDs,
compression settings, frame structures and so on are
all from :
https://www.goes-r.gov/users/docs/PUG-GRB-vol4.pdf
*/

// Forward declarations
namespace goes
{
    namespace grb
    {
        struct GRBFilePayload;
        class GRBDataProcessor;
    }
}

namespace goes
{
    namespace grb
    {
        class GRBDataProcessor
        {
        private:
            // CFG
            const std::string directory;

            // Utils
            std::string timestamp_to_string(double timestamp);
            image::Image<uint16_t> get_image_product(GRBFilePayload &payload);

            // ABI Product processing
            std::map<int, std::shared_ptr<GRBABIImageAssembler>> abi_image_assemblers;
            void processABIImageProduct(GRBFilePayload &payload);
            void processABIImageMeta(GRBFilePayload &payload);

            // SUVI Product processing
            std::map<int, std::shared_ptr<GRBSUVIImageAssembler>> suvi_image_assemblers;
            void processSUVIImageProduct(GRBFilePayload &payload);
            void processSUVIImageMeta(GRBFilePayload &payload);

            // GLM Product processing
            void processGLMData(GRBFilePayload &payload);

            // GRB Information
            void processGRBInformation(GRBFilePayload &payload);

        public:
            GRBDataProcessor(std::string directory);
            ~GRBDataProcessor();
            void processPayload(GRBFilePayload &payload);
        };
    }
}

// Has to be after we define GRBDataProcessor
#include "payload_assembler.h"