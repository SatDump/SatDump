#include "xeuvi_reader.h"
#include "image/io.h"
#include "logger.h"

namespace fengyun3
{
    namespace xeuvi
    {
        XEUVIReader::XEUVIReader(std::string directory) : directory(directory) { image.init(16, 1073, 1035, 1); }

        XEUVIReader::~XEUVIReader() {}

        void XEUVIReader::writeCurrent()
        {
            image::save_img(image, directory + "/XEUVI_" + std::to_string(images_count + 1) + ".png");
            image.fill(0);
            images_count++;
        }

        void XEUVIReader::work(std::vector<uint8_t> &packet)
        {
            int cnt = (packet[34] << 8 | packet[35]) + 1;
            uint8_t marker = packet[10] >> 6;

            if (cnt > 1021 && marker != 1)
                return; // Make sure we don't go out of bounds due to an invalid counter

            if (marker == 2) // End
            {
                for (int i = 0; i < 30044 / 2; i++)
                    if (cnt * 1073 + i < (int)image.size()) // Avoid running out if data is shifted. Still can occur in normal ops!
                        image.set(cnt * 1073 + i, packet[34 + i * 2 + 0] << 8 | packet[34 + i * 2 + 1]);
            }
            else if (marker == 1) // Start
            {
                // Save old
                writeCurrent();

                for (int i = 0; i < 64380 / 2 - 1073; i++)
                    image.set(0 * 1073 + i, packet[536 + 2146 + i * 2 + 0] << 8 | packet[536 + 2146 + i * 2 + 1]);
            }
            else // Middle
            {
                for (int i = 0; i < 64380 / 2; i++)
                    if (cnt * 1073 + i < (int)image.size()) // Avoid running out if data is shifted. Still can occur in normal ops!
                        image.set(cnt * 1073 + i, packet[34 + i * 2 + 0] << 8 | packet[34 + i * 2 + 1]);
            }
        }
    } // namespace xeuvi
} // namespace fengyun3