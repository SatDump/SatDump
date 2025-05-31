#include "timestamp_filtering.h"
#include "logger.h"
#include "utils/stats.h"

namespace satdump
{
    namespace timestamp_filtering
    {
        std::vector<double> filter_timestamps_simple(std::vector<double> timestamps, double max_tolerate, double max_diff)
        {
            std::vector<double> filter_timestamps = timestamps;
            double avg = satdump::get_median(filter_timestamps);
            double last = -1;
            for (double &v : filter_timestamps)
            {
                // logger->critical(abs(avg - v));

                if (v == -1)
                    continue;

                if (abs(avg - v) > max_tolerate)
                {
                    last = v;
                    v = -1;
                    continue;
                }

                if (last >= v || abs(last - v) > max_diff)
                {
                    last = v;
                    v = -1;
                    continue;
                }

                last = v;
            }

            // for (double &v : filter_timestamps)
            //    logger->info(v);

            return filter_timestamps;
        }

        std::vector<double> filter_timestamps_width_cfg(std::vector<double> timestamps, nlohmann::json timestamps_filter)
        {
            // logger->debug("Filtering timestamps..."); TODOREWORK?

            if (timestamps_filter["type"].get<std::string>() == "simple")
            {
                double scan_time = timestamps_filter["scan_time"]; // Does NOT have to be actual scan rate... Just the best name for it.
                double max_diff = timestamps_filter["max_diff"];
                double margin = timestamps_filter.contains("margin") ? timestamps_filter["margin"].get<double>() : 1.5;

                double total_time = scan_time * timestamps.size();
                double final_range = total_time * 0.5 + total_time * margin;

                return filter_timestamps_simple(timestamps, final_range, max_diff);
            }
            else
            {
                return timestamps;
            }
        }
    } // namespace timestamp_filtering
} // namespace satdump