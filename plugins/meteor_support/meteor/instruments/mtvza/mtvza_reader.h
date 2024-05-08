#pragma once

#include <cstdint>
#include <vector>
#include "common/image2/image.h"

namespace meteor
{
    namespace mtvza
    {
        class MTVZAReader
        {
        private:
            std::vector<uint16_t> channels[30];
            void parse_samples(uint8_t *data, int ch_start, int offset, int ch_cnt, int nsamples, int counter);

        public:
            MTVZAReader();
            ~MTVZAReader();

            int lines;
            std::vector<double> timestamps;

            double latest_msumr_timestamp = -1;

            void work(uint8_t *data);
            image2::Image getChannel(int channel);

            bool endian_mode = false;
        };
    }
}