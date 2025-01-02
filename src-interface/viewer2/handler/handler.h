#pragma once

/**
 * @file handler.h
 */

#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <functional>
#include "tree.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace viewer
    {
        /**
         * @brief SatDump's handler base class.
         *
         * Handlers are meant to handle displaying and maniipulating
         * data inside the viewer. To avoid code duplication, they may
         * also serve as headless processors.
         *
         * This implements all basic UI functions and handler tree system,
         * as it is intended for handlers to be able to contain any other
         * handler as a dependency.
         */
        class Handler
        { // TODOREWORK PRIVATE DELETE STUFF
        private:
            TreeDrawerToClean tree_local;
            std::vector<std::shared_ptr<Handler>> subhandlers;
            std::mutex subhandlers_mtx;
            std::vector<std::shared_ptr<Handler>> subhandlers_marked_for_del;

        public:
            /**
             * @brief Render viewer menu left sidebar
             */
            virtual void drawMenu() = 0;

            /**
             * @brief Render viewer contents (center/left)
             */
            virtual void drawContents(ImVec2 win_size) = 0;

            /**
             * @brief Render viewer menu bar (in the left sidebar)
             */
            virtual void drawMenuBar() {}

            /**
             * @brief Render viewer menu bar (in the left sidebar)
             * @param h currently selected handler, to be replaced if
             * another is selected
             */
            virtual void drawTreeMenu(std::shared_ptr<Handler> &h);

            /**
             * @brief Get this handler's readable name
             * @return name as a string
             */
            virtual std::string getName() { return "!!!Invalid!!!"; }

            /**
             * @brief Get this handler's ImGui ID for rendering in the tree
             * @return ID as a string
             */
            std::string getTreeID() { return getName() + "##" + std::to_string((size_t)this); }

            /**
             * @brief Check if this handler contains any sub-handlers
             * @return true if subhandlers are present
             */
            bool hasSubhandlers();

            /**
             * @brief Add a new subhandler
             * @param handler the handler to add
             */
            void addSubHandler(std::shared_ptr<Handler> handler);

            /**
             * @brief Delete a subhandler
             * @param handler the handler to delete
             */
            void delSubHandler(std::shared_ptr<Handler> handler);

        public:
            static std::string getID(); // TODOREWORK
            static std::shared_ptr<Handler> getInstance();
        };

        extern std::map<std::string, std::function<std::shared_ptr<Handler>()>> handlers_registry; // TODOREWORK?
        void registerHandlers();
    }
}