#include "trash_handler.h"

namespace satdump
{
    namespace handlers
    {
        TrashHandler::TrashHandler() { handler_tree_icon = u8"\uf014"; }

        TrashHandler::~TrashHandler() {}

        void TrashHandler::drawMenu() {}

        void TrashHandler::drawMenuBar() {}

        void TrashHandler::drawContents(ImVec2 win_size) {}

        void TrashHandler::setConfig(nlohmann::json p) {}

        nlohmann::json TrashHandler::getConfig()
        {
            nlohmann::json p;
            return p;
        }
    } // namespace handlers
} // namespace satdump
