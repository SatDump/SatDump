#pragma once

#include "common/ccsds/ccsds.h"

#include "common/image/image.h"
#include <string>
#include <map>
#include <memory>

namespace proba
{
    namespace hrc
    {
        class HRCImage
        {
        public:
            unsigned short *tempChannelBuffer;

            HRCImage()
            {
                tempChannelBuffer = new unsigned short[74800 * 12096];
            }

            ~HRCImage()
            {
                delete[] tempChannelBuffer;
            }

            image::Image<uint16_t> getImg()
            {
                return image::Image<uint16_t>(tempChannelBuffer, 1072, 1072, 1);
            }
        };

        class HRCReader
        {
        private:
            std::map<int, std::shared_ptr<HRCImage>> hrc_images;

            std::string output_folder;

        public:
            HRCReader(std::string &outputfolder);
            ~HRCReader();

            int getCount() { return hrc_images.size(); }

            void save();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace hrc
} // namespace proba