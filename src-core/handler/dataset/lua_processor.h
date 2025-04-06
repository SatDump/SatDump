#pragma once

#include "processor.h"
#include "TextEditor.h"

namespace satdump
{
    namespace viewer
    {
        class Lua_DatasetProductProcessor : public DatasetProductProcessor
        {
        private:
            TextEditor editor;

        public:
            Lua_DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p);

            nlohmann::json getCfg();

            bool can_process();
            void process(float *progress = nullptr);
            void renderUI();
        };
    }
}