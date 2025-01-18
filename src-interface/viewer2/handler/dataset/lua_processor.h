#pragma once

#include "processor.h"

namespace satdump
{
    namespace viewer
    {
        class Lua_DatasetProductProcessor : public DatasetProductProcessor
        {
        public:
            Lua_DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p)
                : DatasetProductProcessor(dh, dp, p)
            {
            }

            bool can_process();
            void process(float *progress = nullptr);
        };
    }
}