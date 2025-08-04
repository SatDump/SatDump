#include "module_gk2a_lrit_data_decoder.h"
#include "common/codings/crc/crc_generic.h"
#include "common/utils.h"
#include "core/resources.h"
#include "gk2a/uhrit_demux.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "init.h"
#include "key_decryptor.h"
#include "libs/miniz/miniz.h"
#include "libs/miniz/miniz_zip.h"
#include "logger.h"
#include "lrit_header.h"
#include "nlohmann/json_utils.h"
#include "utils/http.h"
#include "xrit/gk2a/gk2a_headers.h"
#include "xrit/processor/xrit_channel_processor_render.h"
#include "xrit/transport/xrit_demux.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace gk2a
{
    namespace lrit
    {
        GK2ALRITDataDecoderModule::GK2ALRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), write_images(parameters["write_images"].get<bool>()),
              write_additional(parameters["write_additional"].get<bool>()), write_unknown(parameters["write_unknown"].get<bool>())
        {
            fsfsm_enable_output = false;
            processor.directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGES";
            uhrit_mode = getValueOrDefault(parameters["uhrit_mode"], false);
        }

        GK2ALRITDataDecoderModule::~GK2ALRITDataDecoderModule() {}

        void GK2ALRITDataDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            this->directory = directory;

            logger->warn("All credits for decoding GK-2A encrypted xRIT files goes to @sam210723, and xrit-rx over on Github.");
            logger->warn("See https://vksdr.com/xrit-rx for a lot more information!");

            std::string key_path = "";

            if (resources::resourceExists("gk2a/EncryptionKeyMessage.bin")) // Do we have it locally?
            {
                key_path = resources::getResourcePath("gk2a/EncryptionKeyMessage.bin");
            }
            else // We don't attempt to download and decrypt
            {
                std::string out_enc = satdump::user_path + "/encrypted_key.bin";
                std::string out_dec = satdump::user_path + "/gk2a_keys.bin";

                if (!std::filesystem::exists(out_dec))
                {
                    // Try to download
                    std::string download_url = "http://web.archive.org/web/20210502102522if_/http://nmsc.kma.go.kr/resources/enhome/resources/satellites/coms/COMS_Decryption_Sample_Cpp.zip";
                    std::string file_data;
                    if (!satdump::perform_http_request(download_url, file_data))
                    {
                        logger->info("Downloaded COMS Decryption sample! Decompressing...");

                        mz_zip_archive zipFile;
                        MZ_CLEAR_OBJ(zipFile);

                        // Extract ZIP
                        if (mz_zip_reader_init_mem(&zipFile, file_data.c_str(), file_data.size(), MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
                        {
                            if (mz_zip_reader_extract_file_to_file(&zipFile, "EncryptionKeyMessage_001F2904C905.bin", out_enc.c_str(), MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
                            {
                                logger->info("Extracted!");

                                // Decrypt
                                if (!decrypt_key_file(out_enc, "001F2904C905", out_dec))
                                    key_path = out_dec;

                                if (std::filesystem::exists(out_enc))
                                    std::filesystem::remove(out_enc);
                            }
                        }
                    }
                    else
                    {
                        logger->info("Key Download failed!");
                    }
                }
                else
                {
                    key_path = out_dec;
                }
            }

            if (key_path != "")
            {
                // Load decryption keys
                std::ifstream keyFile(key_path, std::ios::binary);

                // Read key count
                uint16_t key_count = 0;
                keyFile.read((char *)&key_count, 2);
                key_count = (key_count & 0xFF) << 8 | (key_count >> 8);

                uint8_t readBuf[10];
                for (int i = 0; i < key_count; i++)
                {
                    keyFile.read((char *)readBuf, 10);
                    int index = readBuf[0] << 8 | readBuf[1];
                    uint64_t key = (uint64_t)readBuf[2] << 56 | (uint64_t)readBuf[3] << 48 | (uint64_t)readBuf[4] << 40 | (uint64_t)readBuf[5] << 32 | (uint64_t)readBuf[6] << 24 |
                                   (uint64_t)readBuf[7] << 16 | (uint64_t)readBuf[8] << 8 | (uint64_t)readBuf[9];
                    std::memcpy(&key, &readBuf[2], 8);
                    decryption_keys.emplace(std::pair<int, uint64_t>(index, key));
                }

                keyFile.close();
            }
            else
            {
                logger->critical("GK-2A Decryption keys could not be loaded!!! Nothing apart from encrypted data will be decoded.");
            }

            logger->info("Demultiplexing and deframing...");

            satdump::xrit::XRITDemux lrit_demux;
            satdump::xrit::UHRITDemux uhrit_demux;

            uhrit_demux.onParseHeader = lrit_demux.onParseHeader = [](satdump::xrit::XRITFile &file) -> void
            {
                if (file.hasHeader<KeyHeader>())
                {
                    KeyHeader key_header = file.getHeader<KeyHeader>();
                    if (key_header.key != 0)
                    {
                        logger->debug("This is encrypted!");
                        file.custom_flags.insert_or_assign(satdump::xrit::gk2a::IS_ENCRYPTED, true);
                        file.custom_flags.insert_or_assign(satdump::xrit::gk2a::KEY_INDEX, key_header.key);
                    }
                    else
                    {
                        file.custom_flags.insert_or_assign(satdump::xrit::gk2a::IS_ENCRYPTED, false);
                    }
                }
                else
                {
                    file.custom_flags.insert_or_assign(satdump::xrit::gk2a::IS_ENCRYPTED, false);
                }
            };

            if (!std::filesystem::exists(directory + "/IMAGES/Unknown"))
                std::filesystem::create_directories(directory + "/IMAGES/Unknown");

            if (uhrit_mode)
            {
                uint8_t cadu[2048];
                codings::crc::GenericCRC crc_check(16, 0x1021, 0xFFFF, 0x0, false, false);

                while (should_run())
                {
                    // Read buffer
                    read_data((uint8_t *)cadu, 2048);

                    uint16_t crc1 = cadu[2046] << 8 | cadu[2047];
                    uint16_t crc2 = crc_check.compute(&cadu[4], 2042);
                    if (crc1 != crc2)
                    {
                        //      logger->critical("CRC -------------------------");
                        continue;
                    }

                    std::vector<satdump::xrit::XRITFile> files = uhrit_demux.work(cadu);
                    for (auto &file : files)
                        processLRITFile(file);
                }
            }
            else
            {
                uint8_t cadu[1024];

                while (should_run())
                {
                    // Read buffer
                    read_data((uint8_t *)&cadu, 1024);

                    std::vector<satdump::xrit::XRITFile> files = lrit_demux.work(cadu);

                    for (auto &file : files)
                        processLRITFile(file);
                }
            }

            cleanup();

            processor.flush();
        }

        void GK2ALRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GK-2A LRIT Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            satdump::xrit::renderAllTabsFromProcessors({&processor});

            drawProgressBar();

            ImGui::End();
        }

        std::string GK2ALRITDataDecoderModule::getID() { return "gk2a_lrit_data_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GK2ALRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GK2ALRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace lrit
} // namespace gk2a