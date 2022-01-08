#include "data_processor.h"
#include <filesystem>
#include "logger.h"
#include "common/image/image.h"
#include "common/image/jpeg_utils.h"
#include "abi/abi_products.h"
#include <fstream>
#include "glm/glm_parser.h"

namespace goes
{
    namespace grb
    {
        GRBDataProcessor::GRBDataProcessor(std::string directory)
            : directory(directory)
        {
            // Setup ABI Image assemblers
            for (std::pair<const int, products::ABI::GRBProductABI> &abi_prod : products::ABI::ABI_IMAGE_PRODUCTS)
                abi_image_assemblers.emplace(abi_prod.first, std::make_shared<GRBABIImageAssembler>(directory + "/ABI", abi_prod.second));

            // Setup SUVI Image assemblers
            for (std::pair<const int, products::SUVI::GRBProductSUVI> &suvi_prod : products::SUVI::SUVI_IMAGE_PRODUCTS)
                suvi_image_assemblers.emplace(suvi_prod.first, std::make_shared<GRBSUVIImageAssembler>(directory + "/SUVI", suvi_prod.second));
        }

        GRBDataProcessor::~GRBDataProcessor()
        {
        }

        std::string GRBDataProcessor::timestamp_to_string(double timestamp)
        {
            time_t time_tt = timestamp;
            std::tm *timeReadable = gmtime(&time_tt);
            std::string utc_filename = std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";
            return utc_filename;
        }

        image::Image<uint16_t> GRBDataProcessor::get_image_product(GRBFilePayload &payload)
        {
            image::Image<uint16_t> img;

            if (payload.sec_header.grb_payload_variant == IMAGE || payload.sec_header.grb_payload_variant == IMAGE_WITH_DQF)
            {
                GRBImagePayloadHeader image_header(&payload.payload[0]);
                int size = image_header.byte_offset_dqf;

                if (image_header.compression_algorithm == NO_COMPRESSION)
                {
                    img = image::Image<uint16_t>((uint16_t *)&payload.payload[34], image_header.image_block_width, image_header.image_block_height - image_header.row_offset_image_block, 1);
                }
                else if (image_header.compression_algorithm == JPEG_2000)
                {
                    img = image::decompress_j2k_openjp2(&payload.payload[34], size);
                }
                else if (image_header.compression_algorithm == SZIP)
                {
                    logger->error("SZIP Compression is not supposed to be used on GRB! Please report this error, support has not been implemented yet.");
                }
            }
            else
            {
                logger->error("Attempted decoding image data, but it's not the right type!");
            }

            return img;
        }

        void GRBDataProcessor::processABIImageProduct(GRBFilePayload &payload)
        {
            if (payload.sec_header.grb_payload_variant == IMAGE || payload.sec_header.grb_payload_variant == IMAGE_WITH_DQF)
            {
                // Extract image block and image header
                GRBImagePayloadHeader image_header(&payload.payload[0]);
                image::Image<uint16_t> block = get_image_product(payload);
                abi_image_assemblers[payload.apid]->pushBlock(image_header, block);
            }
            else
            {
                logger->error("ABI Image product should be of image type!");
            }
        }

        void GRBDataProcessor::processABIImageMeta(GRBFilePayload &payload)
        {
            products::ABI::GRBProductABI abi_product = products::ABI::ABI_IMAGE_PRODUCTS_META[payload.apid];

            if (payload.sec_header.grb_payload_variant == GENERIC)
            {
                GRBGenericPayloadHeader generic_header(&payload.payload[0]);
                std::string zone = products::ABI::abiZoneToString(abi_product.type);
                std::string filename = "ABI_" + zone + "_" + std::to_string(abi_product.channel) + "_" + timestamp_to_string(generic_header.utc_time);
                std::string abi_dir = directory + "/ABI/" + zone + "/" + timestamp_to_string(generic_header.utc_time) + "/";
                std::filesystem::create_directories(abi_dir);
                logger->info("Saving " + abi_dir + filename + ".xml");
                std::ofstream output_xml(abi_dir + filename + ".xml", std::ios::binary);
                output_xml.write((char *)&payload.payload[21], payload.payload.size() - 21);
                output_xml.close();
            }
            else
            {
                logger->error("ABI Metadata should be of generic type!");
            }
        }

        void GRBDataProcessor::processSUVIImageProduct(GRBFilePayload &payload)
        {
            if (payload.sec_header.grb_payload_variant == IMAGE || payload.sec_header.grb_payload_variant == IMAGE_WITH_DQF)
            {
                // Extract image block and image header
                GRBImagePayloadHeader image_header(&payload.payload[0]);
                image::Image<uint16_t> block = get_image_product(payload);
                suvi_image_assemblers[payload.apid]->pushBlock(image_header, block);
            }
            else
            {
                logger->error("SUVI Image product should be of image type!");
            }
        }

        void GRBDataProcessor::processSUVIImageMeta(GRBFilePayload &payload)
        {
            std::string channel = products::SUVI::SUVI_IMAGE_PRODUCTS_META[payload.apid];
            std::string suvidir = directory + "/SUVI/" + channel + "/";
            std::filesystem::create_directories(suvidir);

            if (payload.sec_header.grb_payload_variant == GENERIC)
            {
                GRBGenericPayloadHeader generic_header(&payload.payload[0]);
                std::string filename = "SUVI_" + channel + "_" + timestamp_to_string(generic_header.utc_time);
                logger->info("Saving " + suvidir + filename + ".xml");
                std::ofstream output_xml(suvidir + filename + ".xml", std::ios::binary);
                output_xml.write((char *)&payload.payload[21], payload.payload.size() - 21);
                output_xml.close();
            }
            else
            {
                logger->error("SUVI Metadata should be of generic type!");
            }
        }

        void GRBDataProcessor::processGRBInformation(GRBFilePayload &payload)
        {
            std::filesystem::create_directories(directory + "/Information");

            if (payload.sec_header.grb_payload_variant == GENERIC)
            {
                GRBGenericPayloadHeader generic_header(&payload.payload[0]);
                std::string utc_filename = timestamp_to_string(generic_header.utc_time);
                logger->info("Saving " + directory + "/Information/" + utc_filename + ".xml");
                std::ofstream output_xml(directory + "/Information/" + utc_filename + ".xml", std::ios::binary);
                output_xml.write((char *)&payload.payload[21], payload.payload.size() - 21);
                output_xml.close();
            }
            else
            {
                logger->error("GRB Information should be of generic type!");
            }
        }

        void GRBDataProcessor::processGLMData(GRBFilePayload &payload)
        {
            if (payload.sec_header.grb_payload_variant == GENERIC)
            {
                GRBGenericPayloadHeader generic_header(&payload.payload[0]);
                products::GLM::GRBGLMProductType glm_cfg = products::GLM::GLM_PRODUCTS[payload.apid];

                if (glm_cfg == products::GLM::META)
                {
                    std::string glm_meta_dir = directory + "/GLM/Meta/";
                    std::filesystem::create_directories(glm_meta_dir);
                    std::string utc_filename = timestamp_to_string(generic_header.utc_time);
                    logger->info("Saving " + glm_meta_dir + utc_filename + ".xml");
                    std::ofstream output_xml(glm_meta_dir + utc_filename + ".xml", std::ios::binary);
                    output_xml.write((char *)&payload.payload[21], payload.payload.size() - 21);
                    output_xml.close();
                }
                else
                {
                    std::string glm_data = parseGLMFrameToJSON(&payload.payload[21], payload.payload.size() - 21, glm_cfg);
                    std::string glm_dir = directory + "/GLM/" + (glm_cfg == products::GLM::FLASH ? "Flash" : (glm_cfg == products::GLM::EVENT ? "Event" : "Group")) + "/";
                    std::filesystem::create_directories(glm_dir);
                    std::string utc_filename = timestamp_to_string(generic_header.utc_time);
                    logger->info("Saving " + glm_dir + utc_filename + ".json");
                    std::ofstream output_glm(glm_dir + utc_filename + ".json", std::ios::binary);
                    output_glm.write((char *)glm_data.c_str(), glm_data.size());
                    output_glm.close();
                }
            }
            else
            {
                logger->error("GLM Data should be of generic type!");
            }
        }

        void GRBDataProcessor::processPayload(GRBFilePayload &payload)
        {
            // Check if this is ABI data
            if (products::ABI::ABI_IMAGE_PRODUCTS.count(payload.apid) > 0)
                processABIImageProduct(payload);
            // ...or metadata
            if (products::ABI::ABI_IMAGE_PRODUCTS_META.count(payload.apid) > 0)
                processABIImageMeta(payload);

            // Check if this is SUVI data
            if (products::SUVI::SUVI_IMAGE_PRODUCTS.count(payload.apid) > 0)
                processSUVIImageProduct(payload);
            // ...or metadata
            if (products::SUVI::SUVI_IMAGE_PRODUCTS_META.count(payload.apid) > 0)
                processSUVIImageMeta(payload);

            // Check if this is GLM data
            if (products::GLM::GLM_PRODUCTS.count(payload.apid) > 0)
                processGLMData(payload);

            // Check if this is Information data
            if (payload.apid == 0x580)
                processGRBInformation(payload);
        }
    }
}
