#include "module_noaa_avhrr_gac.h"
#include <fstream>
//#include "avhrr_reader.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
//#include "common/image/earth_curvature.h"
//#include "modules/noaa/noaa.h"
//#include "common/geodetic/projection/satellite_reprojector.h"
//#include "nlohmann/json_utils.h"
//#include "common/geodetic/projection/proj_file.h"
#include "common/utils.h"
//#include "common/image/composite.h"
//#include "common/map/leo_drawer.h"

#include "common/repack.h"

#include "common/image/image.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

bool mod2(bool x, bool y)
{
    return (x + y) % 2;
}

bool rand_bits[1023] = {
    0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1,
    0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0,
    1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0,
    1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0,
    0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1,
    1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0,
    1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0,
    0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0,
    1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1,
    0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
    1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1,
    1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0,
    0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0,
    1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1,
    0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1,
    1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1,
    0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1,
    0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1,
    1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
    1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1,
    0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1,
    0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1,
    1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1,
    0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1,
    1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1,
    0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1,
    1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1,
    1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0,
    1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1,
    1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1,
    1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0,
    1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0,
    0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0,
    0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1,
    1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0,
    1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0,
    1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0,
    0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0,
    1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1,
    1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1};

namespace noaa
{
    namespace avhrr
    {
        NOAAAVHRRGACDecoderModule::NOAAAVHRRGACDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void NOAAAVHRRGACDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // AVHRRReader reader;

            uint8_t buffer[4159];
            uint16_t words[3327];

            logger->info("Demultiplexing and deframing...");

            image::Image<uint16_t> image1(409, 10000, 1);
            image::Image<uint16_t> image2(409, 10000, 1);
            image::Image<uint16_t> image3(409, 10000, 1);
            image::Image<uint16_t> image4(409, 10000, 1);
            image::Image<uint16_t> image5(409, 10000, 1);

            int lines = 0;

            std::ofstream outfile("rand_noaa.bin");

            uint8_t rand_buffer[4159];
            memset(rand_buffer, 0, 4159);

            bool pn_sequence[1023];

            bool repackFrame[33270];

            /*{
                //uint64_t pn_register = 0b111111111111111111111111111111111111111111111111111111111111111;

                //bool fliflops[63];
                //memset(fliflops, 0, 63);

                uint16_t pn_reg = 0xFFFF;

                for (int i = 0; i < 10230; i++)
                {
                    /*bool xor10 = getBit<uint64_t>(pn_register, 6 - 1);
                    bool xor5 = getBit<uint64_t>(pn_register, 5 - 1);
                    bool xor2 = getBit<uint64_t>(pn_register, 2 - 1);
                    bool xor1 = getBit<uint64_t>(pn_register, 63 - 1);
                    //bool xor1 = getBit(pn_register, 1 - 1);
                    bool outBit = xor10 ^ xor5 ^ xor2 ^ xor1; // getBit<uint64_t>(pn_register, 62);

                    pn_register = pn_register << 1 | outBit;
                    //pn_sequence[i] = outBit;
                    logger->info(outBit);

                    //register_pn[0] = register_pn[9];
                    //register_pn[1] = register_pn[9];

                    //bool out = getBit<uint64_t>(pn_register, 0);
                    //for (int y = 0; y < 63; y++)
                    //{
                    //    out ^= getBit<uint64_t>(pn_register, y);
                    //}*/
            /*
                    bool xor1 = getBit(pn_reg, 1 - 1);
                    bool xor3 = getBit(pn_reg, 2 - 1);
                    bool xor5 = getBit(pn_reg, 5 - 1);
                    bool xor8 = getBit(pn_reg, 6 - 1);

                    bool out = xor1 ^ xor3 ^ xor5 ^ xor8;

                    pn_reg = pn_reg << 1 | out;

                    rand_buffer[i / 8] = rand_buffer[i / 8] << 1 | out; //getBit<uint64_t>(pn_register, 0);
                }

                //for (int i = 0; i < 4159 * 8; i++)
                {

                    //i++;
                }
            }*/
            uint8_t derand_pn[4159];
            memset(derand_pn, 0, 4159);

            for (int i = 0; i < 4159 * 8 - 60; i++)
            {
                bool bit = 0;

                bit = rand_bits[i % 1023];

                int real_bit = i + 60; //+ 60;
                                       // if (real_bit >= 60)
                derand_pn[real_bit / 8] = derand_pn[real_bit / 8] << 1 | bit;
            }

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 4159);

                for (int i = 0; i < 4159; i++)
                {
                    buffer[i] ^= derand_pn[i];
                }

                // shift_array_left((char *)buffer, 4159, 2);

                // Repack to bits
                /*{
                    // Repack
                    for (int i = 0; i < 33270; i++)
                    {
                        repackFrame[i] = (buffer[i / 8] >> (0 - (i % 8))) & 1;
                    }

                    // Repack
                    memset(buffer, 0, 4159);
                    for (int i = 0; i < 33270; i++)
                    {
                        buffer[i / 8] = buffer[i / 8] << 1 | repackFrame[i + 0];
                    }
                }*/

                repackBytesTo10bits(buffer, 4159, words);

                /*for (int i = 0; i < 3327 - 6; i++)
                {
                    words[6 + i] = __bswap_16(words[6 + i]);
                }*/

                outfile.write((char *)buffer, 4159);

                // int id = (words[6] >> 7) & 0b11;
                // logger->info(id);

                for (int i = 0; i < 409; i++)
                {
                    int pos = 1182;
                    image1[lines * 409 + i] = words[pos + i * 5 + 0] * 60; //<< 6;
                    image2[lines * 409 + i] = words[pos + i * 5 + 1] * 60; //<< 6;
                    image3[lines * 409 + i] = words[pos + i * 5 + 2] * 60; //<< 6;
                    image4[lines * 409 + i] = words[pos + i * 5 + 3] * 60; //<< 6;
                    image5[lines * 409 + i] = words[pos + i * 5 + 4] * 60; //<< 6;
                    // logger->info(words[pos + i * 5 + 0]);
                }

                lines++;

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            // logger->info("AVHRR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            /*image::Image<uint16_t> image1 = reader.getChannel(0);
            image::Image<uint16_t> image2 = reader.getChannel(1);
            image::Image<uint16_t> image3 = reader.getChannel(2);
            image::Image<uint16_t> image4 = reader.getChannel(3);
            image::Image<uint16_t> image5 = reader.getChannel(4);
            */

            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/AVHRR-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/AVHRR-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/AVHRR-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/AVHRR-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/AVHRR-5.png");

            image::Image<uint16_t> image221(409, 10000, 3);
            image221.draw_image(0, image2);
            image221.draw_image(1, image2);
            image221.draw_image(2, image1);

            logger->info("Channel 5...");
            WRITE_IMAGE(image221, directory + "/AVHRR-221.png");
        }

        void NOAAAVHRRGACDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("NOAA AVHRR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string NOAAAVHRRGACDecoderModule::getID()
        {
            return "noaa_avhrr_gac";
        }

        std::vector<std::string> NOAAAVHRRGACDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> NOAAAVHRRGACDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<NOAAAVHRRGACDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace noaa