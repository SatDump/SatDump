#pragma once

#include "handler/handler.h"

#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"

#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <thread>

#include "imgui/pfd/widget.h"
#include "common/widgets/image_view.h"
#include "common/widgets/menuitem_fileopen.h"

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

            // Transform
            bool use_range = false;

            double range_min = 10;
            double range_max = 50;

            inline double range_transform(double x)
            {
                double size = lut.size();
                return range_min + (x / size) * (range_max - range_min);
            }

            inline double range_transform_inv(double x)
            {
                double size = lut.size();
                return ((x - range_min) / (range_max - range_min)) * size;
            }

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

            // Replicate
            int replicate_times = 2;

            // TODOREWORK File save
            widget::MenuItemImageSave file_save_menu;
            widget::MenuItemFileOpen file_open_menu;

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