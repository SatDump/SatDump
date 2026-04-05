#pragma once

#include "handlers/handler.h"
#include "imgui/imgui.h"
#include "libserial/serial.h"
#include "satdump_downconverter/dc.h"
#include "utils/task_queue.h"
#include <memory>
#include <thread>

namespace satdump
{
    namespace exp_devs
    {
        class SatDumpDownconverHandler : public handlers::Handler
        {
        protected:
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

        private:
            std::shared_ptr<SatDumpDownconverter> dc;
            TaskQueue tq;

            bool updateOnChange = true;

            std::thread fetchTh;
            bool fetchTh_r = true;

            std::vector<serial_cpp::PortInfo> ports;

        private:
            double pll_freq = 0;
            bool pll_lock = false;
            bool pll_enabled = false;

            double attenuation = 0;

            double ref_freq = 0;
            bool ref_ext = false;

            bool preamp_enabled = false;

            double mcu_temp = 0;
            double rf_temp = 0;
            double vreg_temp = 0;

        private:
            void updateDC();

        public:
            SatDumpDownconverHandler();
            ~SatDumpDownconverHandler();

        public:
            std::string getID() { return "satdump_downconverter_handler"; }
            std::string getName() { return "SatDump Downconverter"; }
        };
    } // namespace exp_devs
}; // namespace satdump