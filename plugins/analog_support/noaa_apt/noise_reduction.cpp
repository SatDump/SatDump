#include "noise_reduction.h"
#include "common/dsp/window/window.h"

namespace dsp
{
    AptNoiseReductionBlock::AptNoiseReductionBlock(std::shared_ptr<dsp::stream<complex_t>> input, int bins) : Block(input), _bins(bins)
    {
        // Init buffers
        {
            // Allocate FFT buffers
            forwFFTIn = (complex_t *)fftwf_malloc(_bins * sizeof(complex_t));
            forwFFTOut = (complex_t *)fftwf_malloc(_bins * sizeof(complex_t));
            backFFTIn = (complex_t *)fftwf_malloc(_bins * sizeof(complex_t));
            backFFTOut = (complex_t *)fftwf_malloc(_bins * sizeof(complex_t));

            // Allocate and clear delay buffer
            buffer = dsp::create_volk_buffer<complex_t>(STREAM_BUFFER_SIZE + 64000);
            bufferStart = &buffer[_bins - 1];

            // Clear backward FFT input since only one value is changed and reset at a time
            for (int i = 0; i < _bins; i++)
                backFFTIn[i].imag = backFFTIn[i].real = 0;

            // Allocate amplitude buffer
            ampBuf = dsp::create_volk_buffer<float>(_bins);

            // Allocate and generate Window
            fftWin = dsp::create_volk_buffer<float>(_bins);
            for (int i = 0; i < _bins; i++)
                fftWin[i] = dsp::window::nuttall(i, _bins - 1);

            // Plan FFTs
            forwardPlan = fftwf_plan_dft_1d(_bins, (fftwf_complex *)forwFFTIn, (fftwf_complex *)forwFFTOut, FFTW_FORWARD, FFTW_ESTIMATE);
            backwardPlan = fftwf_plan_dft_1d(_bins, (fftwf_complex *)backFFTIn, (fftwf_complex *)backFFTOut, FFTW_BACKWARD, FFTW_ESTIMATE);
        }
    }

    AptNoiseReductionBlock::~AptNoiseReductionBlock()
    {
        // Destroy buffers
        {
            fftwf_destroy_plan(forwardPlan);
            fftwf_destroy_plan(backwardPlan);
            fftwf_free(forwFFTIn);
            fftwf_free(forwFFTOut);
            fftwf_free(backFFTIn);
            fftwf_free(backFFTOut);
            volk_free(buffer);
            volk_free(ampBuf);
            volk_free(fftWin);
        }
    }

    void AptNoiseReductionBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        process(nsamples, input_stream->readBuf, output_stream->writeBuf);

        input_stream->flush();
        output_stream->swap(nsamples);
    }

    int AptNoiseReductionBlock::process(int count, const complex_t *in, complex_t *out)
    {
        // Write new input data to buffer buffer
        memcpy(bufferStart, in, count * sizeof(complex_t));

        // Iterate the FFT
        for (int i = 0; i < count; i++)
        {
            // Apply windows
            volk_32fc_32f_multiply_32fc((lv_32fc_t *)forwFFTIn, (lv_32fc_t *)&buffer[i], fftWin, _bins);

            // Do forward FFT
            fftwf_execute(forwardPlan);

            // Process bins here
            uint32_t idx;
            volk_32fc_magnitude_32f(ampBuf, (lv_32fc_t *)forwFFTOut, _bins);
            volk_32f_index_max_32u(&idx, ampBuf, _bins);

            // Keep only the bin of highest amplitude
            backFFTIn[idx] = forwFFTOut[idx];

            // Do reverse FFT and get first element
            fftwf_execute(backwardPlan);
            out[i] = backFFTOut[_bins / 2];

            // Reset the input buffer
            backFFTIn[idx] = {0, 0};
        }

        // Move buffer buffer
        memmove(buffer, &buffer[count], (_bins - 1) * sizeof(complex_t));

        return count;
    }
}