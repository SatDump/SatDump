#pragma once

#include "app.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <thread>
#include <vector>

#include "imgui/pfd/widget.h"

#include "bit_container.h"

namespace satdump
{
    class BitViewApplication : public Application
    {
    protected:
        float panel_ratio = 0.23;
        float last_width = -1.0f;

        void drawUI();
        void drawPanel();
        void drawContents();

    private:
        FileSelectWidget select_bitfile_dialog = FileSelectWidget("File", "Select File", false, true);

        std::shared_ptr<BitContainer> current_bit_container;
        std::vector<std::shared_ptr<BitContainer>> all_bit_containers;

    private:
        float process_progress = 0;
        std::thread process_thread;

        std::string deframer_syncword = "0x1acffc1d";
        int deframer_syncword_size = 32;
        int deframer_syncword_framesize = 8192;

    public:
        BitViewApplication();
        ~BitViewApplication();

    public:
        static std::string getID() { return "bitview"; }
        std::string get_name() { return "BitView"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<BitViewApplication>(); }
    };
};