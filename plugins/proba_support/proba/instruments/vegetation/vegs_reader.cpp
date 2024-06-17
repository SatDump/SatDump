#include "vegs_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

#include "logger.h"

namespace proba
{
    namespace vegetation
    {
        VegetationS::VegetationS(int frm_size, int line_size)
        {
            img_data.resize(line_size);

            lines = 0;

            this->frm_size = frm_size;
            this->line_size = line_size;

            tmp_words = new uint16_t[line_size * 2];
        }

        VegetationS::~VegetationS()
        {
            delete[] tmp_words;
        }

        void VegetationS::work(ccsds::CCSDSPacket &packet)
        {
            if ((int)packet.payload.size() < frm_size)
                return;

            repackBytesTo12bits(&packet.payload[18], frm_size - 18, tmp_words);

            for (int i = 0; i < line_size; i++)
                img_data[lines * line_size + i] = tmp_words[i] << 4;

            lines++;
            img_data.resize((lines + 1) * line_size);
        }

        image::Image VegetationS::getImg()
        {
            return image::Image(img_data.data(), 16, line_size, lines, 1);
        }
    } // namespace swap
} // namespace proba