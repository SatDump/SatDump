#pragma once

#include "../app.h"

#include "common/widgets/menuitem_fileopen.h"
#include "handlers/handler.h"

#include "imgui/dialogs/widget.h"

namespace satdump
{
    namespace viewer
    {
        class ViewerApplication : public Application
        {
        public:
            struct RenderLoadMenuElementsEvent
            {
                std::shared_ptr<handlers::Handler> &curr_handler;
                std::shared_ptr<handlers::Handler> &master_handler;
            };

        protected:
            const std::string app_id;
            void drawUI();

            float panel_ratio = 0.23;
            float last_width = -1.0f;

            void drawPanel();
            void drawContents();
            void drawMenuBar();

            // Viewer main handlers
            std::shared_ptr<handlers::Handler> curr_handler;
            std::shared_ptr<handlers::Handler> master_handler;
            std::shared_ptr<handlers::Handler> trash_handler;

            // File open
            widget::MenuItemFileOpen file_open_dialog;
            std::thread file_open_thread; // TODOREWORK?

            std::string quickOpenString;

        public:
            // TODOREWORK last opened by time
            std::map<std::string, std::shared_ptr<handlers::Handler>> last_selected_handler;

            struct GetLastSelectedOfTypeEvent
            {
                std::string type;
                std::shared_ptr<handlers::Handler> &h;
            };

        public:
            void tryOpenFileInViewer(std::string path);

        public:
            ViewerApplication();
            ~ViewerApplication();

        public:
            static std::string getID() { return "viewer"; }
            std::string get_name() { return "Viewer"; }
            static std::shared_ptr<Application> getInstance() { return std::make_shared<ViewerApplication>(); }
        };
    } // namespace viewer
}; // namespace satdump