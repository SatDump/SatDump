#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include <string>

namespace meteosat
{
    namespace msg
    {
        class SEVIRIReader
        {
        private:
            bool d_mode_is_rss;

            image::Image images_nrm[11];
            image::Image images_hrv;

            uint16_t tmp_linebuf_nrm[15000];

            double last_timestamp = 0;
            std::vector<int> all_scids;

        public:
            int lines_since_last_end;
            int not_channels_lines;

            std::string d_directory;

            // UI Stuff
            bool hasToUpdate = false;
            unsigned int textureID = 0;
            uint32_t *textureBuffer;
            bool is_saving = false;

        public:
            SEVIRIReader(bool d_mode_is_rss);
            ~SEVIRIReader();
            void work(int scid, ccsds::CCSDSPacket &pkt);

            void saveImages();
        };
    }
}