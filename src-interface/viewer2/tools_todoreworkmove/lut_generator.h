#pragma once

#include "products2/handler/handler.h"

#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"

#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <thread>

#include "imgui/pfd/widget.h"
#include "common/widgets/image_view.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace viewer
    {
        class LutGeneratorHandler : public Handler
        {
        private:
            struct ColorPX
            {
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;
                uint8_t a = 255;
            };

            ColorPX current_col;
            std::vector<ColorPX> lut;

            int mouse_mode = 0;
            ColorPX interpolate_start_color;
            int interpolate_start_position = 0;

            // Image gen
            FileSelectWidget image_select = FileSelectWidget("Preview Image", "Preview Image");
            bool should_regen_image = false;
            ImageViewWidget preview_img;

            ColorPX getColorFromScreen();

        public:
            LutGeneratorHandler();
            ~LutGeneratorHandler();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Lut Generator"; }

            std::string getID() { return "lut_generator_handler"; }
        };
    }
}