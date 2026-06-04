#include "m10_parser.h"
#include "m10.h"
#include "utils/time.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        M10ParserBlock::M10ParserBlock() : Block("m10_parser_h", {{"in", DSP_SAMPLE_TYPE_S8}}, {}) {}

        void M10ParserBlock::init() {}

        M10ParserBlock::~M10ParserBlock() {}

        bool M10ParserBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            radiosonde::M10Frame *frm = (radiosonde::M10Frame *)iblk.getSamples<uint8_t>();

            if (frm->type == 0x9F)
            {
                radiosonde::M10Frame_9f *f = (radiosonde::M10Frame_9f *)frm;

                const uint32_t ms = f->time[0] << 24 | f->time[1] << 16 | f->time[2] << 8 | f->time[3];
                const uint16_t week = f->week[0] << 8 | f->week[1];

                time_t time = (ms / 1000UL) + ((86400UL * 7) * week) + 315964800UL;

                this->time = timestamp_to_string(time);

                this->lat = (int32_t(f->lat[0] << 24 | f->lat[1] << 16 | f->lat[2] << 8 | f->lat[3]) * 360.0) / ((uint64_t)1UL << 32);
                this->lon = (int32_t(f->lon[0] << 24 | f->lon[1] << 16 | f->lon[2] << 8 | f->lon[3]) * 360.0) / ((uint64_t)1UL << 32);
                this->alt = int32_t(f->alt[0] << 24 | f->alt[1] << 16 | f->alt[2] << 8 | f->alt[3]) / 1e3;
            }
            else if (frm->type == 0x20) {}

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump