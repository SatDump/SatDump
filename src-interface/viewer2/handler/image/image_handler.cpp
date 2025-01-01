#include "image_handler.h"

#include "imgui/imgui_stdlib.h"
#include "core/style.h"

namespace satdump
{
    namespace viewer
    {
        ImageHandler::ImageHandler()
        {
        }

        ImageHandler::~ImageHandler()
        {
        }

        void ImageHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Image"))
            {
            }
        }

        void ImageHandler::drawContents(ImVec2 win_size)
        {
            image_view.draw(win_size);
        }
    }
}
