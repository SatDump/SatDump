#pragma once

#include "processor.h"
#include "flowgraph/flowgraph.h"

namespace satdump
{
    namespace viewer
    {
        class Flowgraph_DatasetProductProcessor : public DatasetProductProcessor
        {
        private:
            Flowgraph flowgraph;

        public:
            Flowgraph_DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p);

            nlohmann::json getCfg();

            bool can_process();
            void process(float *progress = nullptr);
            void renderUI();
        };
    }
}