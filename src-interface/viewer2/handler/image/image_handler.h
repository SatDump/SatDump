#pragma once

#include "../handler.h"

#include "logger.h"
#include "common/widgets/image_view.h"

namespace satdump
{
    namespace viewer
    {
        class ImageHandler : public Handler
        {
        public:
            ImageHandler();
            ~ImageHandler();

            ImageViewWidget image_view;

            void updateImage(image::Image &img)
            {
                image_view.update(img);
            }

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "ImageTODOREWORK"; }

            static std::string getID() { return "image_handler"; }
            static std::shared_ptr<Handler> getInstance() { return std::make_shared<ImageHandler>(); }
        };
    }
}