#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "common/image/image.h"
#include <fftw3.h>
#include <volk/volk.h>

namespace scisat1
{
    namespace fts
    {
        class FTSReader
        {
        private:
            int num_samples = 65526 / 2;

            std::vector<uint16_t> img_data;

            fftwf_complex *fftw_in;
            fftwf_complex *fftw_out;
            fftwf_plan fftw_plan;

            float *fft_output_buffer;

        public:
            FTSReader();
            ~FTSReader();

            int lines = 0;

            void work(ccsds::CCSDSPacket &packet);
            image::Image getImg();
        };
    } // namespace swap
} // namespace proba