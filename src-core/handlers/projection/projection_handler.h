#pragma once

#include "../handler.h"
#include "../image/image_handler.h"
#include "proj_ui.h"

namespace satdump
{
    namespace handlers
    {
        class ProjectionHandler : public Handler, public ProcessingHandler
        {
        public:
            ProjectionHandler();
            ~ProjectionHandler();

            std::string proj_name = "Projection";

            ImageHandler img_handler;

            proj::ProjectionConfigUI projui;

            // Auto-update in UI
            bool needs_to_update = false;

            // Proc function
            void do_process();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            void delSubHandler(std::shared_ptr<Handler> handler, bool now = false) { Handler::delSubHandler(handler, now); }

            std::string getName() { return proj_name; }
            std::string getID() { return "projection_handler"; }
        };
    } // namespace handlers
} // namespace satdump