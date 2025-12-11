#pragma once

#include "tool.h"

namespace satdump
{
    class DifferentialTool : public BitViewTool
    {
    private:
        int diff_mode = 0;
        bool should_process = false;

    public:
        std::string getName();
        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy);
        bool needToProcess();
        void setProcessed();
        void process(std::shared_ptr<BitContainer> &container, float &process_progress);
    };
} // namespace satdump