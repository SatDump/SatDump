#include "audio_sink.h"

namespace satdump
{
    namespace ndsp
    {
        AudioSinkBlock::AudioSinkBlock()
            // : Block(std::is_same_v<T, complex_t> ? "audio_sink_c" : "audio_sink_f",
            //         {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
            //         {{}})
            : Block("audio_sink_f", {{"in", DSP_SAMPLE_TYPE_F32}}, {})
        {
        }

        AudioSinkBlock::~AudioSinkBlock() {}

        bool AudioSinkBlock::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                iblk.free();
                audio_sink->stop();
                return true;
            }

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            auto samples = DSPBuffer::newBufferSamples<float>(iblk.max_size);
            float *floats = iblk.getSamples<float>();
            int16_t *res_samples = samples.getSamples<int16_t>();
            int16_t *audio_samples = samples.getSamples<int16_t>();

            volk_32f_s32f_convert_16i(res_samples, (float *)floats, 32767, iblk.size);

            audio_sink->resample_s16(audio_samples, res_samples, samplerate, audio_samplerate, iblk.size, 1);

            if (play_audio && enable_audio)
                audio_sink->push_samples(audio_samples, iblk.size);

            iblk.free();

            return false;
        }
    } // namespace ndsp
} // namespace satdump
