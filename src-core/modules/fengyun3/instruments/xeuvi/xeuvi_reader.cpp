#include "xeuvi_reader.h"
#include "logger.h"

namespace fengyun3
{
    namespace xeuvi
    {
        XEUVIReader::XEUVIReader(std::string directory) : directory(directory)
        {
            lines = 0;
            image = image::Image<uint16_t>(1073, 1032 * 4, 1);
        }

        XEUVIReader::~XEUVIReader()
        {
        }

        void XEUVIReader::writeCurrent()
        {
            image.crop(1, 0, 1072, lines);

            // Make it a bit more... Readable?
            for (int i = 0; i < image.height() * image.width(); i++)
                image[i] = std::max(0, std::min(65535, (image[i] - 50) * 255));

            image.save_png(std::string(directory + "/XEUVI_" + std::to_string(images_count++ + 1) + ".png").c_str());
            logger->info("Saving XEUVI image to" + directory + "/XEUVI_" + std::to_string(images_count++ + 1) + ".png");
            image = image::Image<uint16_t>(1073, 1032 * 4, 1);
            lines = 0;
        }

        void XEUVIReader::work(std::vector<uint8_t> &packet)
        {
            uint8_t marker = packet[10] >> 6;

            if (marker == 2)
            {
                for (int i = 0; i < 30044 / 2; i++)
                    image[lines * 1073 + i] = packet[34 + i * 2 + 0] << 8 | packet[34 + i * 2 + 1];
                lines += 12;
            }
            else if (marker == 1) // Start
            {
                // Save old
                if (lines > 0)
                    writeCurrent();

                for (int i = 0; i < 64380 / 2 - 1073; i++)
                    image[lines * 1073 + i] = packet[536 + 2146 + i * 2 + 0] << 8 | packet[536 + 2146 + i * 2 + 1];
                lines += 28;
            }
            else
            {
                for (int i = 0; i < 64380 / 2; i++)
                    image[lines * 1073 + i] = packet[34 + i * 2 + 0] << 8 | packet[34 + i * 2 + 1];
                lines += 29;
            }

            lines++;
        }
    } // namespace virr
} // namespace fengyun