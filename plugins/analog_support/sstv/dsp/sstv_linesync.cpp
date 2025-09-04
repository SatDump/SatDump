#include "sstv_linesync.h"
#include "dsp/block.h"
#include "logger.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <volk/volk_malloc.h>

namespace satdump
{
    namespace ndsp
    {
        SSTVLineSyncBlock::SSTVLineSyncBlock() : Block("sstv_linesync_ff", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        SSTVLineSyncBlock::~SSTVLineSyncBlock()
        {
            if (sync_line_buffer != nullptr)
                delete[] sync_line_buffer;
        }

        bool SSTVLineSyncBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            for (uint32_t is = 0; is < iblk.size; is++)
            {
                if (skip > 1)
                {
                    int to_move = std::min<int>(iblk.size - is, skip);
                    memmove(&sync_line_buffer[0], &sync_line_buffer[to_move], (sync_line_buffer_len - to_move) * sizeof(float));
                    memcpy(&sync_line_buffer[sync_line_buffer_len - to_move], &iblk.getSamples<float>()[is], to_move * sizeof(float));
                    is += to_move - 1;
                    skip -= to_move;
                }
                else
                {
                    memmove(&sync_line_buffer[0], &sync_line_buffer[1], (sync_line_buffer_len - 1) * sizeof(float));
                    sync_line_buffer[sync_line_buffer_len - 1] = iblk.getSamples<float>()[is];
                    skip--;
                }

                if (skip > 0)
                    continue;

                int best_cor = sync.size() * 255;
                int best_pos = 0;
                for (int pos = 0; pos < line_length; pos++)
                {
                    int cor = 0;
                    for (size_t i = 0; i < sync.size(); i++)
                    {
                        float fv = sync_line_buffer[pos + i];
                        fv = (fv - minval) / (maxval - minval);

                        if (fv < 0)
                            fv = 0;
                        if (fv > 1)
                            fv = 1;

                        cor += abs(int(fv * 255) - sync[i]);
                    }

                    if (cor < best_cor)
                    {
                        best_cor = cor;
                        best_pos = pos;
                    }
                }

                auto oblk = outputs[0].fifo->newBufferSamples<float>(line_length);
                oblk.size = line_length;
                for (int i = 0; i < line_length; i++)
                    oblk.getSamples<float>()[i] = lineGetScaledIMG(sync_line_buffer[best_pos + i]);
                outputs[0].fifo->wait_enqueue(oblk);

                skip = line_length;
            }

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump