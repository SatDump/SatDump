#pragma once

#include "tool.h"

namespace satdump
{
    class TakeSkipTool : public BitViewTool
    {
    private:
        int skip_bits = 1;
        bool should_process = false;

    public:
        std::string getName();
        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy);
        bool needToProcess();
        void setProcessed();
        void process(std::shared_ptr<BitContainer> &container, float &process_progress);
    };
} // namespace satdump