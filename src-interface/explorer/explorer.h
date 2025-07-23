#pragma once

#include "common/widgets/menuitem_fileopen.h"
#include "dsp/task_queue.h"
#include "handlers/handler.h"

#include "imgui/dialogs/widget.h"

namespace satdump
{
    namespace explorer
    {
        class ExplorerApplication
        {
        public:
            struct RenderLoadMenuElementsEvent
            {
                std::shared_ptr<handlers::Handler> &curr_handler;
                std::shared_ptr<handlers::Handler> &master_handler;
            };

        public:
            void draw();

        protected:
            const std::string app_id;

            float panel_ratio = 0.23;
            float last_width = -1.0f;

            void drawPanel();
            void drawContents();
            void drawMenuBar();

            // Explorer main handlers
            std::shared_ptr<handlers::Handler> curr_handler;
            std::shared_ptr<handlers::Handler> master_handler;
            std::shared_ptr<handlers::Handler> processing_handler;
            std::shared_ptr<handlers::Handler> processing_handler_sub;
            std::shared_ptr<handlers::Handler> trash_handler;

            // File open
            widget::MenuItemFileOpen file_open_dialog;
            TaskQueue file_open_queue;

            std::string quickOpenString;

        public:
            // TODOREWORK last opened by time
            std::map<std::string, std::shared_ptr<handlers::Handler>> last_selected_handler;

        public:
            void tryOpenFileInExplorer(std::string path);

        public:
            ExplorerApplication();
            ~ExplorerApplication();
        };

        // Events
        struct GetLastSelectedOfTypeEvent
        {
            std::string type;
            std::shared_ptr<handlers::Handler> &h;
        };

        struct ExplorerAddHandlerEvent
        {
            std::shared_ptr<handlers::Handler> h;
            bool open = false;
            bool is_processing = false;
        };
    } // namespace explorer
}; // namespace satdump