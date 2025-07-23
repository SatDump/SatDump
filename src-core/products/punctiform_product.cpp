#include "punctiform_product.h"
#include "logger.h"
#include "common/tracking/tracking.h"
#include "core/exception.h"

namespace satdump
{
    namespace products
    {
        void PunctiformProduct::save(std::string directory)
        {
            type = "punctiform"; // TODOREWORK rename all this...

            contents["data"] = data;

            Product::save(directory);
        }

        void PunctiformProduct::load(std::string file)
        {
            Product::load(file);
            std::string directory = std::filesystem::path(file).parent_path().string();

            data = contents["data"];
        }

        geodetic::geodetic_coords_t PunctiformProduct::get_sample_position(int ch_index, int sample_index)
        {
            if (ch_index >= data.size())
                throw satdump_exception("Invalid channel " + std::to_string(ch_index) + "!");

            auto &ch = data[ch_index];

            if (sample_index > ch.data.size())
                throw satdump_exception("Invalid sample index " + std::to_string(sample_index) + " our of " + std::to_string(ch.data.size()) + "!");

            if (ch.positions.size() > 0)
            {
                return ch.positions[sample_index];
            }
            else if (has_tle()) /*TODOREWORK EPHEM vs TLE must be generic */
            {
                if (satellite_tracker == nullptr)
                    satellite_tracker = new satdump::SatelliteTracker((satdump::TLE)get_tle());
                satdump::SatelliteTracker *t = (satdump::SatelliteTracker *)satellite_tracker;
                return t->get_sat_position_at(ch.timestamps[sample_index]);
            }

            throw satdump_exception("No positions or TLE/Ephem to provide sample position!");
        }

        PunctiformProduct::~PunctiformProduct()
        {
            if (satellite_tracker != nullptr)
                delete (satdump::SatelliteTracker *)satellite_tracker;
        }
    }
}