#include "DecompWT/CompressT4.h"
#include "DecompWT/CompressWT.h"
#include "image/io.h"
#include "image/jpeg_utils.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "lrit_header.h"
#include "module_elektro_lrit_data_decoder.h"
#include "utils/string.h"
#include "xrit/identify.h"
#include "xrit/msg/decomp.h"
#include <filesystem>

namespace elektro
{
    namespace lrit
    {
        void ELEKTROLRITDataDecoderModule::processLRITFile(satdump::xrit::XRITFile &file)
        {
            std::string current_filename = file.filename;

            satdump::xrit::PrimaryHeader primary_header = file.getHeader<satdump::xrit::PrimaryHeader>();

            satdump::xrit::XRITFileInfo finfo = satdump::xrit::identifyXRITFIle(file);

            // Check if this is image data, and if so also write it as an image
            if (finfo.type != satdump::xrit::XRIT_UNKNOWN)
            {
                if (primary_header.file_type_code == 0 && file.hasHeader<satdump::xrit::ImageStructureRecord>())
                    if (finfo.type == satdump::xrit::XRIT_ELEKTRO_MSUGS || finfo.type == satdump::xrit::XRIT_MSG_SEVIRI)
                        satdump::xrit::msg::decompressMsgHritFileIfRequired(file);

                processor.push(finfo, file);
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
} // namespace elektro