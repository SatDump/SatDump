#pragma once

#include "../app.h"

#include "handler/handler.h"

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
            };

        protected:
            const std::string app_id;
            void drawUI();

            float panel_ratio = 0.23;
            float last_width = -1.0f;

            void drawPanel();
            void drawContents();
            void drawMenuBar();

            std::shared_ptr<Handler> curr_handler;
            std::shared_ptr<Handler> master_handler;
            std::shared_ptr<Handler> trash_handler;

            // TODOREWORK File open
            std::thread file_open_thread;
            fileutils::FileSelTh *file_open_dialog = nullptr;

        public:
            void openProductOrDataset(std::string path);

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