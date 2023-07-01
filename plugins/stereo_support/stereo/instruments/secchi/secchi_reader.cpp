#include "secchi_reader.h"
#include <filesystem>
#include "common/utils.h"
#include "resources.h"

namespace stereo
{
    namespace secchi
    {
        SECCHIReader::SECCHIReader(std::string icer_path, std::string output_directory)
            : icer_path(icer_path), output_directory(output_directory)
        {
        }

        std::string filename_timestamp(double timestamp)
        {
            const time_t timevalue = timestamp;
            std::tm *timeReadable = gmtime(&timevalue);
            return std::to_string(timeReadable->tm_year + 1900) + "-" +
                   (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                   (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                   (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                   (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                   (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
        }

        void SECCHIReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.header.apid == 1137) // Cor2
            {
                for (auto b : secchi_assembler0.work(pkt))
                {
                    if (b.hdr.block_type == 1) // Header
                    {
                        auto hdr = read_base_hdr(b.payload.data());
                        last_timestamp_0 = hdr.actualExpTime;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_icer_tool(&b.payload[0], b.hdr.block_length, 256);

                        for (size_t i = 0; i < img.size(); i++)
                            img[i] <<= 2;

                        uint16_t text_color[] = {65535, 65535, 65535, 65535};

                        img.white_balance();
                        img.to_rgba();
                        img.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "COR2";

                        img.draw_text(150 / 2, 460 / 2, text_color, 30 / 2, timestamp_to_string(last_timestamp_0));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        if (last_timestamp_0 != 0)
                            img.save_png(output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_0) + ".png");
                        else
                            img.save_png(output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++) + ".png");

                        last_timestamp_0 = 0;
                    }
                }
            }
            else if (pkt.header.apid == 1138) // Cor?
            {
                for (auto b : secchi_assembler1.work(pkt))
                {
                    if (b.hdr.block_type == 1) // Header
                    {
                        auto hdr = read_base_hdr(b.payload.data());
                        last_timestamp_1 = hdr.actualExpTime;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_icer_tool(&b.payload[0], b.hdr.block_length, 256);

                        for (size_t i = 0; i < img.size(); i++)
                            img[i] <<= 2;

                        uint16_t text_color[] = {65535, 65535, 65535, 65535};

                        img.white_balance();
                        img.to_rgba();
                        img.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "Unknown";

                        img.draw_text(150 / 2, 460 / 2, text_color, 30 / 2, timestamp_to_string(last_timestamp_1));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        // if (last_timestamp_1 != 0)
                        //     img.save_png(output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_1) + ".png");
                        // else
                        img.save_png(output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++) + ".png");

                        last_timestamp_1 = 0;
                    }
                }
            }
            else if (pkt.header.apid == 1139) // Cor?
            {
                for (auto b : secchi_assembler2.work(pkt))
                {
                    if (b.hdr.block_type == 1) // Header
                    {
                        auto hdr = read_base_hdr(b.payload.data());
                        last_timestamp_2 = hdr.actualExpTime;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_icer_tool(&b.payload[0], b.hdr.block_length, 512);

                        for (size_t i = 0; i < img.size(); i++)
                            img[i] <<= 2;

                        uint16_t text_color[] = {65535, 65535, 65535, 65535};

                        img.white_balance();
                        img.to_rgba();
                        img.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "Unknown";

                        img.draw_text(150 / 2, 460 / 2, text_color, 30 / 2, timestamp_to_string(last_timestamp_2));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        // if (last_timestamp_2 != 0)
                        //     img.save_png(output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_2) + ".png");
                        // else
                        img.save_png(output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++) + ".png");

                        last_timestamp_2 = 0;
                    }
                }
            }
            else if (pkt.header.apid == 1140) // EUVI 195 / 304
            {
                for (auto b : secchi_assembler3.work(pkt))
                {
                    if (b.hdr.block_type == 1) // Header
                    {
                        auto hdr = read_base_hdr(b.payload.data());
                        last_timestamp_3 = hdr.actualExpTime;
                        last_polarization_3 = hdr.actualPolarPosition;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_icer_tool(&b.payload[0], b.hdr.block_length, 512);

                        for (size_t i = 0; i < img.size(); i++)
                            img[i] <<= 3;

                        uint16_t text_color[] = {65535, 65535, 65535, 65535};

                        img.white_balance();
                        img.to_rgba();
                        img.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "Unknown";

                        if (last_polarization_3 == 10)
                            channel_name = "EUVI_195";
                        else if (last_polarization_3 == 4)
                            channel_name = "EUVI_304";
                        else
                            channel_name = "Corrupted";

                        img.draw_text(150, 460, text_color, 30, timestamp_to_string(last_timestamp_3));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        if (last_timestamp_3 != 0)
                            img.save_png(output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_3) + ".png");
                        else
                            img.save_png(output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++) + ".png");

                        last_timestamp_3 = 0;
                        last_polarization_3 = 0;
                    }
                }
            }
        }

        image::Image<uint16_t> SECCHIReader::decompress_icer_tool(uint8_t *data, int dsize, int size)
        {
            std::ofstream("./stereo_secchi_raw.tmp").write((char *)data, dsize);

            if (std::filesystem::exists("./stereo_secchi_out.tmp"))
                std::filesystem::remove("./stereo_secchi_out.tmp");

            std::string cmd = icer_path + " -vv -i ./stereo_secchi_raw.tmp -o ./stereo_secchi_out.tmp";

            if (!std::filesystem::exists(icer_path))
            {
                logger->error("No ICER Decompressor provided. Can't decompress SECCHI!");
                return image::Image<uint16_t>();
            }

            int ret = system(cmd.data());

            if (ret == 0 && std::filesystem::exists("./stereo_secchi_out.tmp"))
            {
                logger->trace("SECCHI Decompression OK!");

                std::ifstream data_in("./stereo_secchi_out.tmp", std::ios::binary);
                uint16_t *buffer = new uint16_t[size * size];
                data_in.read((char *)buffer, sizeof(uint16_t) * size * size);
                image::Image<uint16_t> img(buffer, size, size, 1);
                delete[] buffer;

                if (std::filesystem::exists("./stereo_secchi_out.tmp"))
                    std::filesystem::remove("./stereo_secchi_out.tmp");

                return img;
            }
            else
            {
                logger->error("Failed decompressing SECCHI!");

                if (std::filesystem::exists("./stereo_secchi_out.tmp"))
                    std::filesystem::remove("./stereo_secchi_out.tmp");

                return image::Image<uint16_t>();
            }
        }
    }
}