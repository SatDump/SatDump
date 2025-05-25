#include "abi_image_composer.h"
#include <ctime>
#include <filesystem>
#include "logger.h"
#include "core/resources.h"

namespace goes
{
    namespace grb
    {
        ABIComposer::ABIComposer(std::string dir, products::ABI::ABIScanType abi_type)
            : abi_directory(dir),
              abi_product_type(abi_type)
        {
            current_timestamp = 0;
            reset();
        }

        ABIComposer::~ABIComposer()
        {
            if (has_data())
                save();
        }

        void ABIComposer::reset()
        {
            for (int i = 0; i < 16; i++)
            {
                channel_images[i].clear();
                has_channels[i] = false;
            }
        }

        bool ABIComposer::has_data()
        {
            for (int i = 0; i < 16; i++)
                if (has_channels[i])
                    return true;
            return false;
        }

        void ABIComposer::feed_channel(double timestamp, int ch, image::Image &img)
        {
            if (ch >= 16)
                return;

            if (timestamp != current_timestamp)
            {
                if (has_data())
                    save();
                reset();
                current_timestamp = timestamp;
            }

            channel_images[ch - 1] = img;
            has_channels[ch - 1] = true;

            // logger->critical("Channel compose got %d at %f", ch, timestamp);
        }

        void ABIComposer::saveABICompo(image::Image img, std::string name)
        {

            std::string zone = products::ABI::abiZoneToString(abi_product_type);
            time_t time_tt = current_timestamp;
            std::tm *timeReadable = gmtime(&time_tt);
            std::string utc_filename = std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";

            std::string filename = "ABI_" + zone + "_" + name + "_" + utc_filename;
            std::string directory = abi_directory + "/" + zone + "/" + utc_filename + "/";
            std::filesystem::create_directories(directory);

            // img.save_img(std::string(directory + filename).c_str());
            saving_thread->push(img, std::string(directory + filename));
        }

        void ABIComposer::save()
        {
            // Check if we can make 1-3-5
            if (has_channels[0] && has_channels[2] && has_channels[4])
            {
                logger->debug("Generating RGB135 composite...");
                image::Image compo(16, channel_images[0].width(), channel_images[0].height(), 3);
                compo.draw_image(0, channel_images[4]);
                compo.draw_image(1, channel_images[2]);
                compo.draw_image(2, channel_images[0]);
                saveABICompo(compo, "RGB135");
            }

            // Check if we can make 2-14, for a composite with LHCP only
            if (has_channels[1] && has_channels[14])
            {
                logger->debug("Generating False Color 2 & 14 composite...");
                image::Image compo(8, channel_images[1].width(), channel_images[1].height(), 3);

                // Resize CH14 to the same res as ch2
                image::Image ch14 = channel_images[13].to8bits();
                ch14.resize(channel_images[1].width(), channel_images[1].height());

                // Convert CH2 to 8-bits
                image::Image ch2 = channel_images[1].to8bits();

                image::Image ch2_curve, fc_lut;
                image::load_png(ch2_curve, resources::getResourcePath("lut/goes/abi/wxstar/ch2_curve.png").c_str());
                image::load_png(fc_lut, resources::getResourcePath("lut/goes/abi/wxstar/lut.png").c_str());

                for (size_t i = 0; i < ch2.width() * ch2.height(); i++)
                {
                    uint8_t x = ch2_curve.get(ch2.get(i));
                    uint8_t y = std::max(0, (255 - ch14.get(i)) - 69);
                    for (int c = 0; c < 3; c++)
                        compo.set(c * compo.width() * compo.height() + i, fc_lut.get(c * fc_lut.width() * fc_lut.height() + x * fc_lut.width() + y));
                }

                ch2.clear();
                ch14.clear();

                saveABICompo(compo, "LUT214");
            }
        }
    }
}