#pragma once

#include "bitview.h"
#include "libs/ctpl/ctpl_stl.h"

namespace satdump
{
    class BitViewTool
    {
    public:
        virtual std::string getName() = 0;
        virtual void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy) = 0;
        virtual bool needToProcess() = 0;
        virtual void setProcessed() = 0;
        virtual void process(std::shared_ptr<BitContainer> &container, float &process_progress) = 0;
    };
}