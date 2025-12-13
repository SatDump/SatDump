#pragma once

#include "tool.h"

namespace satdump
{
    class DeframerTool : public BitViewTool
    {
    private:
        std::string deframer_syncword = "0x1acffc1d";
        int deframer_syncword_size = 32;
        int deframer_syncword_framesize = 8192;
        int deframer_current_frames = 0;
        int deframer_threshold = 0;
        bool deframer_byte_aligned = false;
        bool deframer_soft_bits_in = false;

        bool should_process = false;

    public:
        std::string getName();
        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy);
        bool needToProcess();
        void setProcessed();
        void process(std::shared_ptr<BitContainer> &container, float &process_progress);
    };
} // namespace satdump