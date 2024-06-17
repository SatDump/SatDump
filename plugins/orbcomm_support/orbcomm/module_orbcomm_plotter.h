#pragma once

#include "core/module.h"

namespace orbcomm
{
    class OrbcommPlotterModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        struct OrbComEphem
        {
            time_t time;
            int scid;
            float az, el;
        };

        std::mutex all_ephem_points_mtx;
        std::vector<OrbComEphem> all_ephem_points;
        std::vector<OrbComEphem> last_ephems;

        bool should_be_a_window = false;

    public:
        OrbcommPlotterModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}
