#pragma once

#include "handler.h"

namespace satdump
{
    namespace viewer
    {
        class DummyHandler : public Handler
        { // TODOREWORK PRIVATE DELETE STUFF
        public:
            void init() {}
            void drawMenu() {}
            void drawContents(ImVec2 win_size) {}

            std::string name = "!!!Dummy!!!";
            std::string getName() { return name; }

            static std::string getID() { return "dummy"; }
            static std::shared_ptr<Handler> getInstance() { return std::make_shared<DummyHandler>(); }
        };
    }
}