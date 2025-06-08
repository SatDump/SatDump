#include "secchi_reader.h"
#include <filesystem>
#include "common/utils.h"
#include "core/resources.h"
#include "utils/time.h"

#include "rice_decomp.h"

#include "image/processing.h"
#include "image/text.h"
#include "image/io.h"

namespace stereo
{
    namespace secchi
    {
        SECCHIReader::SECCHIReader(std::string icer_path, std::string output_directory)
            : icer_path(icer_path), output_directory(output_directory)
        {
            decompression_status_out = std::ofstream(output_directory + "/image_status.txt", std::ios::binary);
        }

        SECCHIReader::~SECCHIReader()
        {
            decompression_status_out.close();
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

        void rotate_image(image::Image &img)
        {
            image::Image img2 = img;
            for (int x = 0; x < (int)img.width(); x++)
            {
                for (int y = 0; y < (int)img.height(); y++)
                {
                    img.set(y * img.width() + x, img2.get(x * img.width() + y));
                }
            }
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
                        last_filename_0 = hdr.filename;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_icer_tool(&b.payload[0], b.hdr.block_length, 256);

                        for (size_t i = 0; i < img.size(); i++)
                            img.set(i, img.get(i) << 2);

                        rotate_image(img);

                        std::vector<double> text_color = {1, 1, 1, 1};

                        image::white_balance(img);
                        img.to_rgba();
                        image::TextDrawer text_drawer;
                        text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "COR2";

                        text_drawer.draw_text(img, 150 / 2, 460 / 2, text_color, 30 / 2, satdump::timestamp_to_string(last_timestamp_0));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        if (last_timestamp_0 != 0)
                            image::save_img(img, output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_0));
                        else
                            image::save_img(img, output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++));

                        if (last_filename_0.size() > 0)
                            decompression_status_out << channel_name << "     " << last_filename_0 << " " << satdump::timestamp_to_string(last_timestamp_0) << " " << ((img.size() > 0) ? "PASS" : "FAIL") << "\n";
                        last_filename_0 = "";
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
                        last_filename_1 = hdr.filename;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_icer_tool(&b.payload[0], b.hdr.block_length, 256);

                        for (size_t i = 0; i < img.size(); i++)
                            img.set(i, img.get(i) << 2);

                        std::vector<double> text_color = {1, 1, 1, 1};

                        image::white_balance(img);
                        img.to_rgba();
                        image::TextDrawer text_drawer;
                        text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "HI1";

                        text_drawer.draw_text(img, 150 / 2, 460 / 2, text_color, 30 / 2, satdump::timestamp_to_string(last_timestamp_1));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        // if (last_timestamp_1 != 0)
                        //     img.save_img(output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_1));
                        // else
                        image::save_img(img, output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++));

                        if (last_filename_1.size() > 0)
                            decompression_status_out << channel_name << "      " << last_filename_1 << " " << satdump::timestamp_to_string(last_timestamp_1) << " " << ((img.size() > 0) ? "PASS" : "FAIL") << "\n";
                        last_filename_1 = "";
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
                        last_filename_2 = hdr.filename;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_rice_tool(&b.payload[0], b.hdr.block_length, 256);

                        std::vector<double> text_color = {1, 1, 1, 1};

                        image::white_balance(img);
                        img.to_rgba();
                        image::TextDrawer text_drawer;
                        text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "HI2";

                        text_drawer.draw_text(img, 150 / 2, 460 / 2, text_color, 30 / 2, satdump::timestamp_to_string(last_timestamp_2));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        // if (last_timestamp_2 != 0)
                        //     img.save_img(output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_2));
                        // else
                        image::save_img(img, output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++));

                        if (last_filename_2.size() > 0)
                            decompression_status_out << channel_name << "      " << last_filename_2 << " " << satdump::timestamp_to_string(last_timestamp_2) << " " << ((img.size() > 0) ? "PASS" : "FAIL") << "\n";
                        last_filename_2 = "";
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
                        last_filename_3 = hdr.filename;
                    }

                    if (b.hdr.block_type == 0) // Image
                    {
                        auto img = decompress_icer_tool(&b.payload[0], b.hdr.block_length, 512);

                        for (size_t i = 0; i < img.size(); i++)
                            img.set(i, img.get(i) << 3);

                        rotate_image(img);
                        img.mirror(false, true);

                        std::vector<double> text_color = {1, 1, 1, 1};

                        image::white_balance(img);
                        img.to_rgba();
                        image::TextDrawer text_drawer;
                        text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));

                        std::string channel_name = "Unknown";

                        if (last_polarization_3 == 10)
                            channel_name = "EUVI_195";
                        else if (last_polarization_3 == 4)
                            channel_name = "EUVI_304";
                        else
                            channel_name = "Corrupted";

                        text_drawer.draw_text(img, 150, 460, text_color, 30, satdump::timestamp_to_string(last_timestamp_3));

                        std::filesystem::create_directories(output_directory + "/" + channel_name);

                        logger->info("Saving SECCHI " + channel_name + " Image");

                        if (last_timestamp_3 != 0)
                            image::save_img(img, output_directory + "/" + channel_name + "/" + filename_timestamp(last_timestamp_3));
                        else
                            image::save_img(img, output_directory + "/" + channel_name + "/" + std::to_string(unknown_cnt++));

                        if (last_filename_3.size() > 0)
                            decompression_status_out << channel_name << " " << last_filename_3 << " " << satdump::timestamp_to_string(last_timestamp_3) << " " << ((img.size() > 0) ? "PASS" : "FAIL") << "\n";
                        last_filename_3 = "";
                        last_timestamp_3 = 0;
                        last_polarization_3 = 0;
                    }
                }
            }
        }

        image::Image SECCHIReader::decompress_icer_tool(uint8_t *data, int dsize, int size)
        {
            std::ofstream("./stereo_secchi_raw.tmp").write((char *)data, dsize);

            if (std::filesystem::exists("./stereo_secchi_out.tmp"))
                std::filesystem::remove("./stereo_secchi_out.tmp");

            std::string cmd = icer_path + /*-vv*/ " -i ./stereo_secchi_raw.tmp -o ./stereo_secchi_out.tmp";

            if (!std::filesystem::exists(icer_path))
            {
                logger->error("No ICER Decompressor provided. Can't decompress SECCHI!");
                return image::Image();
            }

            int ret = system(cmd.data());

            if (ret == 0 && std::filesystem::exists("./stereo_secchi_out.tmp"))
            {
                logger->trace("SECCHI Decompression OK!");

                std::ifstream data_in("./stereo_secchi_out.tmp", std::ios::binary);
                uint16_t *buffer = new uint16_t[size * size];
                data_in.read((char *)buffer, sizeof(uint16_t) * size * size);
                image::Image img(buffer, 16, size, size, 1);
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

                return image::Image();
            }
        }

        image::Image SECCHIReader::decompress_rice_tool(uint8_t *data, int dsize, int size)
        {
#if !RICE_MEMORY_VERSION
            std::ofstream("./stereo_secchi_raw.tmp").write((char *)data, dsize);

            if (std::filesystem::exists("./stereo_secchi_out.tmp"))
                std::filesystem::remove("./stereo_secchi_out.tmp");

            std::string cmd = rice_path + /*-vv*/ " ./stereo_secchi_raw.tmp ./stereo_secchi_out.tmp";

            if (!std::filesystem::exists(rice_path))
            {
                logger->error("No RICE Decompressor provided. Can't decompress SECCHI!");
                return image::Image<uint16_t>();
            }

            soho_compression::SOHORiceDecompressor decomp;
            /*            int ret = decomp.run_main("./stereo_secchi_raw.tmp", "./stereo_secchi_out.tmp"); // system(cmd.data());

                        if (ret == 0 && decomp.output_was_valid && std::filesystem::exists("./stereo_secchi_out.tmp"))
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
                        */

            uint16_t *buffer_img;
            int nrow, ncol;
            int ret = decomp.run_main("./stereo_secchi_raw.tmp", &buffer_img, &nrow, &ncol);

            // logger->info("%d %d", ncol, nrow);

            if (ret == 0 && buffer_img != nullptr && ncol == size && nrow == size)
            {
                image::Image<uint16_t> img(buffer_img, ncol, nrow, 1);
                // delete[] buffer_img;
                return img;
            }
            else
            {
                // delete[] buffer_img;
                return image::Image<uint16_t>();
            }
#else
            soho_compression::SOHORiceDecompressor decomp;
            uint16_t *buffer_img;
            int nrow, ncol;
            int ret = decomp.run_main_buff(data, dsize, &buffer_img, &nrow, &ncol);

            // logger->info("%d %d", ncol, nrow);

            if (ret == 0 && buffer_img != nullptr && ncol == size && nrow == size)
            {
                image::Image img(buffer_img, 16, ncol, nrow, 1);
                // delete[] buffer_img;
                return img;
            }
            else
            {
                // delete[] buffer_img;
                return image::Image();
            }
#endif
        }
    }
}