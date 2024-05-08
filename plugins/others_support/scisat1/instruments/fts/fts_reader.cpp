#include "fts_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

#include "logger.h"

namespace scisat1
{
    namespace fts
    {
        FTSReader::FTSReader()
        {
            img_data.resize(num_samples);

            // Init FFTW
            fftw_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * num_samples);
            fftw_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * num_samples);
            fftw_plan = fftwf_plan_dft_1d(num_samples, fftw_in, fftw_out, FFTW_FORWARD, FFTW_ESTIMATE);

            // Output buffer
            fft_output_buffer = new float[num_samples];
        }

        FTSReader::~FTSReader()
        {
            fftwf_free(fftw_in);
            fftwf_free(fftw_out);
            fftwf_destroy_plan(fftw_plan);
            delete[] fft_output_buffer;
        }

        void FTSReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() >= 65536)
            {
                // I am NOT 100% sure this is how it should be processed, but as far as my experience with processing
                // Fourrier spectrometer data... Should be somewhat OK?
                // Though it's limb-sounding, so hard to judge as-is.
                volk_8i_s32f_convert_32f_u((float *)fftw_in, (int8_t *)&packet.payload[6], num_samples * 2, 127); // Convert to complex
                fftwf_execute(fftw_plan);                                                                         // Run FFT
                volk_32fc_s32f_power_spectrum_32f(fft_output_buffer, (lv_32fc_t *)fftw_out, 1, num_samples);      // To power

                for (int i = 0; i < num_samples; i++)
                {
                    float v = (fft_output_buffer[i] + 100.0) * 1000;
                    if (v < 0)
                        v = 0;
                    if (v > 65535)
                        v = 65535;
                    img_data[lines * num_samples + i] = v;
                }

                lines++;
                img_data.resize((lines + 1) * num_samples);
            }
        }

        image::Image FTSReader::getImg()
        {
            return image::Image(img_data.data(), 16, num_samples, lines, 1);
        }
    } // namespace swap
} // namespace proba