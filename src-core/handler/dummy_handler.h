#pragma once

/**
 * @file dummy_handler.h
 */

#include "handler.h"

namespace satdump
{
    namespace viewer
    {
        /**
         * @brief Dummy handler
         *
         * This handler does absolutely nothing except exist.
         * It is meant to simply allow storing sub-handlers in a submenu
         * in the tree, in order to organize things when neccessary.
         */
        class DummyHandler : public Handler
        {
        private:
            std::string name;

        public:
            /**
             * @brief Constructor, meant to pass the name
             * @param name the intended readable name
             */
            DummyHandler(std::string name) : name(name) {}

            void drawMenu() {}
            void drawContents(ImVec2 win_size) {}

            std::string getName() { return name; }

            std::string getID() { return "dummy"; }
        };
    }
}