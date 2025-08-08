#include "image/io.h"
#include "image/jpeg12_utils.h"
#include "image/jpeg_utils.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "lrit_header.h"
#include "module_gk2a_lrit_data_decoder.h"
#include "utils/string.h"
#include "xrit/gk2a/decomp.h"
#include "xrit/gk2a/gk2a_headers.h"
#include <filesystem>
#include <fstream>

extern "C"
{
#include "libtom/tomcrypt_des.c"
}

namespace gk2a
{
    namespace lrit
    {
        void GK2ALRITDataDecoderModule::processLRITFile(satdump::xrit::XRITFile &file)
        {
            std::string current_filename = file.filename;

            satdump::xrit::PrimaryHeader primary_header = file.getHeader<satdump::xrit::PrimaryHeader>();

            if (file.custom_flags[satdump::xrit::gk2a::IS_ENCRYPTED] && decryption_keys.size() > 0) // Decrypt
            {
                logger->info("Decrypting....");
                uint8_t inblock[8], outblock[8];

                int payloadSize = (file.lrit_data.size() - primary_header.total_header_length);

                // Do we need to pad the last block?
                int pad = payloadSize % 8;
                if (pad != 0)
                {
                    for (int i = 0; i < pad; i++)
                        file.lrit_data.push_back(0x00);
                }

                int blocks = (payloadSize + pad) / 8;

                uint64_t key = decryption_keys[file.custom_flags[satdump::xrit::gk2a::KEY_INDEX]];

                // Init a libtom instance
                symmetric_key sk;
                des_setup((unsigned char *)&key, 8, 0, &sk);

                uint8_t *encrypted_ptr = &file.lrit_data[primary_header.total_header_length];

                // Decrypt
                for (int b = 0; b < blocks; b++)
                {
                    std::memcpy(inblock, &encrypted_ptr[b * 8], 8);

                    // Has to be ran twice to do the entire thing
                    des_ecb_decrypt(inblock, outblock, &sk);
                    des_ecb_decrypt(inblock, outblock, &sk);

                    std::memcpy(&encrypted_ptr[b * 8], outblock, 8);
                }

                // Erase compression header
                file.lrit_data[file.all_headers[KeyHeader::TYPE]] = 0; // Type
            }

            // CHeck if we lack decryption, if so, we're stuck
            if (file.custom_flags[satdump::xrit::gk2a::IS_ENCRYPTED] && decryption_keys.size() <= 0)
            {
                if (!std::filesystem::exists(directory + "/LRIT_ENCRYPTED"))
                    std::filesystem::create_directory(directory + "/LRIT_ENCRYPTED");

                logger->info("Writing file " + directory + "/LRIT_ENCRYPTED/" + file.filename + "...");

                // Write file out
                std::ofstream fileo(directory + "/LRIT_ENCRYPTED/" + file.filename, std::ios::binary);
                fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                fileo.close();
                return;
            }

            satdump::xrit::XRITFileInfo finfo = satdump::xrit::identifyXRITFIle(file);

            // Check if this is image data, and if so also write it as an image
            if (finfo.type != satdump::xrit::XRIT_UNKNOWN)
            {
                if (primary_header.file_type_code == 0 && file.hasHeader<satdump::xrit::ImageStructureRecord>())
                    if (finfo.type == satdump::xrit::XRIT_GK2A_AMI)
                        satdump::xrit::gk2a::decompressGK2AHritFileIfRequired(file);

                processor.push(finfo, file);
            }
            else if (primary_header.file_type_code == 255)
            {
                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions

                int offset = primary_header.total_header_length;

                std::vector<std::string> header_parts = satdump::splitString(current_filename, '_');

                // Get type name
                std::string name = "";
                if (header_parts.size() >= 2)
                    name = header_parts[1] + "/";

                // Get extension
                std::string extension = "bin";
                if (std::string((char *)&file.lrit_data[offset + 1], 3) == "PNG")
                    extension = "png";
                else if (std::string((char *)&file.lrit_data[offset], 3) == "GIF")
                    extension = "gif";
                else if (name == "ANT")
                    extension = "txt";

                if (!std::filesystem::exists(directory + "/ADD/" + name))
                    std::filesystem::create_directories(directory + "/ADD/" + name);

                logger->info("Writing file " + directory + "/ADD/" + name + clean_filename + "." + extension + "...");

                // Write file out
                std::ofstream fileo(directory + "/ADD/" + name + clean_filename + "." + extension, std::ios::binary);
                fileo.write((char *)&file.lrit_data[offset], file.lrit_data.size() - offset);
                fileo.close();
            }
            else
            {
                if (!std::filesystem::exists(directory + "/LRIT"))
                    std::filesystem::create_directory(directory + "/LRIT");

                logger->info("Writing file " + directory + "/LRIT/" + file.filename + "...");

                // Write file out
                std::ofstream fileo(directory + "/LRIT/" + file.filename, std::ios::binary);
                fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                fileo.close();
            }
        }
    } // namespace lrit
} // namespace gk2a