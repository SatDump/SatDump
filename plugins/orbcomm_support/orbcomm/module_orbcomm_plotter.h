#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace orbcomm
{
    class OrbcommPlotterModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
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

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace orbcomm
