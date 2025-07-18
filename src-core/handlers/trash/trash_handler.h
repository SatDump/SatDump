#pragma once

/**
 * @file trash_handler.h
 */

#include "../handler.h"

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief A trash handler.
         *
         * Does what it says. Dragging anything on here
         * will simply discard it!
         */
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

            void addSubHandler(std::shared_ptr<Handler> handler, bool ontop = false)
            {
                // Do nothing!
            }

            std::string getName() { return "Trash"; }

            std::string getID() { return "trash_handler"; }
        };
    } // namespace handlers
} // namespace satdump