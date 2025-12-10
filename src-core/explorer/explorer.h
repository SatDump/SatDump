#pragma once

#include "common/widgets/menuitem_fileopen.h"
#include "dsp/task_queue.h"
#include "handlers/handler.h"

#include "imgui/dialogs/widget.h"
#include <cstdint>
#include <memory>

namespace satdump
{
    namespace explorer
    {
        // Predef
        class ExplorerApplication;

        // Events
        struct RenderLoadMenuElementsEvent
        {
            std::shared_ptr<handlers::Handler> &curr_handler;
            std::shared_ptr<handlers::Handler> &master_handler;
        };

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

        struct ExplorerRequestFileLoad
        {
            std::string path;
            std::vector<std::pair<std::string, std::function<void(std::string, ExplorerApplication *)>>> &loaders;
        };

        // Actual explorer
        class ExplorerApplication
        {
        public:
            void draw();

        protected:
            const std::string app_id;

            float panel_ratio = 0.23;
            float last_width = -1.0f;

            void drawPanel();
            void drawContents();
            void drawMenuBar();

            // Groups definitions. TODOREWORK don't hardcode
            std::map<std::string, std::vector<std::string>> group_definitions = {
                {"Recorders", {"recorder", "newrec_test_handler"}},
                {"Products", {"dataset_handler", "image_product_handler", "punctiform_product_handler"}},
            };

            // Explorer main handlers
            std::shared_ptr<handlers::Handler> curr_handler;
            std::shared_ptr<handlers::Handler> processing_handler;
            std::map<std::string, std::shared_ptr<handlers::Handler>> groups_handlers;
            std::shared_ptr<handlers::Handler> master_handler;

            // File open
            widget::MenuItemFileOpen file_open_dialog;
            TaskQueue file_open_queue;

            std::string quickOpenString;

        protected:
            intptr_t satdump_logo_texture = 0;
            std::string tip_of_the_day = "The tip of the day is that this tip failed to load... Sorry about that.";

        public:
            // TODOREWORK last opened by time
            std::map<std::string, std::shared_ptr<handlers::Handler>> last_selected_handler;

        public:
            void addHandler(std::shared_ptr<handlers::Handler> h, bool open = false, bool is_processing = false);
            void tryOpenFileInExplorer(std::string path);
            void tryOpenSomethingInExplorer(std::function<void(ExplorerApplication *)> f);

        public:
            ExplorerApplication();
            ~ExplorerApplication();
        };

    } // namespace explorer
}; // namespace satdump