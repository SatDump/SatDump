#pragma once

/**
 * @file handler.h
 */

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"
#include "tree.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief SatDump's handler base class.
         *
         * Handlers are meant to handle displaying and maniipulating
         * data inside the explorer. To avoid code duplication, they may
         * also serve as headless processors.
         *
         * This implements all basic UI functions and handler tree system,
         * as it is intended for handlers to be able to contain any other
         * handler as a dependency.
         */
        class Handler
        {
        protected: // TODOREWORK?
            std::vector<std::shared_ptr<Handler>> subhandlers;
            std::mutex subhandlers_mtx;

        public:
            std::string handler_tree_icon = "N";

        private:
            TreeDrawer tree_local;
            std::vector<std::shared_ptr<Handler>> subhandlers_marked_for_del;

            bool handler_can_be_dragged = true;
            bool handler_can_be_dragged_to = true;
            bool handler_can_subhandlers_be_dragged = true;

            bool handler_can_be_reorg = false;

            void delSubHandlersNow()
            {
                subhandlers_mtx.lock();
                if (subhandlers_marked_for_del.size())
                {
                    for (auto &h : subhandlers_marked_for_del)
                        subhandlers.erase(std::find(subhandlers.begin(), subhandlers.end(), h));
                    subhandlers_marked_for_del.clear();
                }
                subhandlers_mtx.unlock();
            }

            bool isParent(const std::shared_ptr<Handler> &dragged, const std::shared_ptr<Handler> &potential_child);

        public:
            /**
             * @brief Render explorer menu left sidebar
             */
            virtual void drawMenu() = 0;

            /**
             * @brief Render explorer contents (center/left)
             */
            virtual void drawContents(ImVec2 win_size) = 0;

            /**
             * @brief Render explorer menu bar (in the left sidebar)
             */
            virtual void drawMenuBar() {}

            /**
             * @brief Render explorer menu bar (in the left sidebar)
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
             * @param ontop if true, adds the handler at the start of the list
             */
            virtual void addSubHandler(std::shared_ptr<Handler> handler, bool ontop = false);

            /**
             * @brief Delete a subhandler
             * @param handler the handler to delete
             */
            virtual void delSubHandler(std::shared_ptr<Handler> handler, bool now = false);

            /**
             * @brief Get all current subhandlers
             * @return List of all subhandlers
             */
            virtual std::vector<std::shared_ptr<Handler>> getAllSubHandlers();

            /**
             * @brief Set if a handler can be dragged around in the tree
             * @param v true to have it be draggable
             */
            void setCanBeDragged(bool v) { handler_can_be_dragged = v; }

            /**
             * @brief Set if a handler can be dragged to in the tree
             * @param v true to have it be a valid drag target
             */
            void setCanBeDraggedTo(bool v) { handler_can_be_dragged_to = v; }

            /**
             * @brief Set if a handler's subhandlers can be dragged to in the tree
             * @param v true to have them be draggable
             */
            void setSubHandlersCanBeDragged(bool v) { handler_can_subhandlers_be_dragged = v; }

            /**
             * @brief Set if a handler's subhandlers can be reordered
             * @param v true to have them be reorderable
             */
            void setCanSubBeReorgTo(bool v) { handler_can_be_reorg = v; }

            /**
             * @brief Optional, allows setting a configuration/state from JSON
             * @param p JSON object
             */
            virtual void setConfig(nlohmann::json p) {}

            /**
             * @brief Optional, allows getting a configuration/state as JSON
             * @return JSON object
             */
            virtual nlohmann::json getConfig();

            /**
             * @brief Optional, allows resetting the handler's configuration
             */
            virtual void resetConfig() {};

        public:
            virtual std::string getID() = 0; // TODOREWORK
        };
    } // namespace handlers
} // namespace satdump