#pragma once

#include "../handler.h"

#include "common/map/shapefile.h"
#include "common/image/image.h"

namespace satdump
{
    namespace viewer
    {
        class TrashHandler : public Handler
        {
        public:
            TrashHandler();
            ~TrashHandler();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            void addSubHandler(std::shared_ptr<Handler> handler)
            {
                // Do nothing!
            }

            std::string getName() { return "Trash"; }

            std::string getID() { return "trash_handler"; }
        };
    }
}