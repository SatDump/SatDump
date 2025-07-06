#pragma once

#include "common/colormaps.h"
#include "common/widgets/image_view.h"
#include "common/widgets/menuitem_fileopen.h"
#include "core/resources.h"
#include "dsp/device/options_displayer_warper.h"
#include "dsp/fft/fft_gen.h"
#include "dsp/io/file_source.h"
#include "dsp/io/iq_types.h"
#include "handlers/handler.h"
#include "imgui/dialogs/widget.h"
#include "imgui/imgui.h"
#include "utils/time.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace satdump
{
    namespace spectrogram
    {
        class SpectrogramHandler : public handlers::Handler
        {
        protected:
            std::mutex work_mtx;
            bool is_busy = false;
            bool active = false;

            std::string fileName;

            int seconds;

            double timestamp;
            double timestampAndDuration;

            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            bool buffer_alloc(size_t size);

        private:
            // Spectrogram image specific thingies :3
            bool has_to_update = false;
            unsigned int textureID = 0;
            uint32_t *textureBuffer = nullptr;
            const size_t d_nLinesToUpdate = 10;
            const int resolution = 65536;

            std::vector<uint32_t> palette;

            int fft_img_i_mod = 0;
            int fft_img_i = 0;
            int last_x_pos = 0;
            int last_y_pos = 0;
            int x_pos;
            int y_pos;

        private:
            // FileSelectWidget select_baseband_dialog = FileSelectWidget("File", "Select File", false, true);
            widget::MenuItemFileOpen file_open_menu;
            widget::MenuItemImageSave image_save_menu;

        public:
            SpectrogramHandler();
            ~SpectrogramHandler();
            void process();
            void stop();
            void applyParams();
            void push_fft_floats(float *values);
            void set_palette(colormaps::Map selectedPalette, bool mutex = true);
            void set_rate(int input_rate, int output_rate);

            std::shared_ptr<ndsp::FFTGenBlock> fft_gen;
            std::shared_ptr<ndsp::FileSourceBlock> file_source;
            std::shared_ptr<ndsp::OptDisplayerWarper> file_opts;

            widgets::NotatedNum<uint64_t> samprate = widgets::NotatedNum<uint64_t>("Samplerate", 6e6, "S/s");

            std::string bbType;

            ndsp::IQType iqType;

            uint64_t samplerate = 6e6;
            bool stereo = true;
            int bbChannels = 2;
            int sampleSize = 8;

            int fft_size = 8192;
            int fft_overlap = 1;
            int selected_fft_size = 8;
            std::vector<int> fft_sizes_lut = {2097152, 1048576, 524288, 262144, 131072, 65536, 32768, 16384, 8192, 4096, 2048, 1024};
            float scale_min = -160;
            float scale_max = 160;

            std::vector<colormaps::Map> palettes;
            std::string palettes_str;
            int selected_palette = 0;

            uint64_t nLines = 512;
            int block_size = 131072;

        public:
            std::string getID() { return "spectrogram_handler"; }
            std::string getName() { return "WIP Spectrogram"; }
        };
    } // namespace spectrogram
} // namespace satdump
