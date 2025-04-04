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

            bool lutaction_started = false;

            image::Image generateLutImage();

            // Image gen
            FileSelectWidget image_select = FileSelectWidget("Preview Image", "Preview Image");
            bool should_regen_image = false;
            ImageViewWidget preview_img;

            // Helper
            ColorPX getColorFromScreen();

            // History
            std::vector<std::vector<ColorPX>> history_vector;

            void saveHistory() { history_vector.push_back(lut); }
            void restoreHistory()
            {
                if (history_vector.size() == 0)
                    return;
                lut = history_vector[history_vector.size() - 1];
                history_vector.erase(history_vector.end() - 1);
                should_regen_image = true;
            }

            // TODOREWORK File save
            bool file_save_thread_running = false;
            std::thread file_save_thread;

        public:
            LutGeneratorHandler();
            ~LutGeneratorHandler();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            std::string getName() { return "Lut Generator"; }

            std::string getID() { return "lut_generator_handler"; }
        };
    }
}