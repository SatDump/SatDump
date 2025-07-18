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

            ImageHandler img_handler;

            proj::ProjectionConfigUI projui;

            // Auto-update in UI
            bool needs_to_update = true;

            // Proc function
            void do_process();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            void delSubHandler(std::shared_ptr<Handler> handler, bool now = false)
            {
                //  img_handler.delSubHandler(handler, true);
                Handler::delSubHandler(handler, now);
            }

            // void drawTreeMenu(std::shared_ptr<Handler> &h)
            // {
            //     img_handler.drawTreeMenu(h);
            //     Handler::drawTreeMenu(h);
            // }

            std::string getName() { return "ProjectionToRename"; }
            std::string getID() { return "projection_handler"; }
        };
    } // namespace handlers
} // namespace satdump