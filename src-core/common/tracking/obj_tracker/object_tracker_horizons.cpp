#include "object_tracker.h"
#include "logger.h"
#include "common/utils.h"
#include "utils/http.h"

namespace satdump
{
    void ObjectTracker::loadHorizons(double curr_time)
    {
        logger->trace("Pulling Horizons data...");

        double start_time = curr_time - 24 * 3600;
        double stop_time = curr_time + 48 * 3600;

        auto hdata = pullHorizonsData(start_time, stop_time, 17280);

        if (hdata.size() > 0)
        {
            horizons_data = hdata;
            last_horizons_fetch_time = curr_time;
            logger->trace("Done pulling Horizons data...");
        }
        else
            logger->trace("Pulled 0 Horizons objects!");
    }

    std::vector<ObjectTracker::HorizonsV> ObjectTracker::pullHorizonsData(double start_time, double stop_time, int num_points)
    {
        std::vector<HorizonsV> hdata;

        std::string cmd = (std::string) "https://ssd.jpl.nasa.gov/api/horizons.api?format=text" +
                          "&OBJ_DATA=NO" +
                          "&MAKE_EPHEM=YES" +
                          "&COMMAND=" + std::to_string(horizonsoptions[current_horizons_id].first) +
                          "&CAL_FORMAT=JD" +
                          "&EPHEM_TYPE=OBSERVER" +
                          "&CENTER='coord@399'" +
                          "&COORD_TYPE=GEODETIC" +
                          "&SITE_COORD='" + (qth_lon >= 0 ? "+" : "") + std::to_string(qth_lon) + "," +
                          (qth_lat >= 0 ? "+" : "") + std::to_string(qth_lat) + "," +
                          std::to_string(qth_alt / 1e3) + "'" +
                          "&START_TIME='JD%20" + std::to_string((start_time / 86400.0) + 2440587.5) + "'" +
                          "&STOP_TIME='JD%20" + std::to_string((stop_time / 86400.0) + 2440587.5) + "'" +
                          "&STEP_SIZE='" + std::to_string(num_points) + "'" + // 17280
                          "&QUANTITIES='4,20'";

        std::string req_result;

        int err = perform_http_request(cmd, req_result);

        if (err != 0)
        {
            logger->error("Could not fetch data from Horizons!");
            return hdata;
        }

        std::istringstream req_results(req_result);
        std::string line;

        bool fount_soe = false;
        bool fount_eoe = false;
        while (getline(req_results, line))
        {
            if (!fount_soe)
            {
                if (line.find("$$SOE") != std::string::npos)
                    fount_soe = true;
                continue;
            }

            if (fount_eoe)
            {
                continue;
            }
            else
            {
                if (line.find("$$EOE") != std::string::npos)
                    fount_eoe = true;
            }

            double julian_time = 0;
            double az = 0;
            double el = 0;
            double delta = 0;
            double deldot = 0;
            double unk = 0;

            if (sscanf(line.c_str(), "%lf%*s    %lf %lf %lf  %lf  %lf",
                       &julian_time, &az, &el, &delta, &deldot, &unk) == 6 ||
                sscanf(line.c_str(), "%lf %*s   %lf %lf %lf  %lf  %*f",
                       &julian_time, &az, &el, &delta, &deldot) == 5 ||
                sscanf(line.c_str(), "%lf %lf %lf %lf  %lf",
                       &julian_time, &az, &el, &delta, &deldot) == 5 ||
                sscanf(line.c_str(), "%lf    %lf %lf %lf  %lf  %lf",
                       &julian_time, &az, &el, &delta, &deldot, &unk) == 6)
            {
                double ctime = (julian_time - 2440587.5) * 86400.0;
                // logger->info("%s %f %f", timestamp_to_string(ctime).c_str(), az, el);
                hdata.push_back({ctime, (float)az, (float)el});
            }
        }

        return hdata;
    }

    std::vector<std::pair<int, std::string>> ObjectTracker::pullHorizonsList()
    {
        std::vector<std::pair<int, std::string>> vv;

        logger->trace("Pulling object list from Horizons...");

        std::string url = "https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='*'";
        std::string req_result;
        satdump::perform_http_request(url, req_result);

        std::istringstream req_results(req_result);
        std::string line;

        bool got_start = false;
        while (getline(req_results, line))
        {
            if (line.find("  -------  ---------------------------------- -----------  ------------------- ") != std::string::npos)
            {
                got_start = true;
                continue;
            }

            if (!got_start)
                continue;

            try
            {
                std::string numid = line.substr(0, 9);
                std::string name = line.substr(11, 35);
                std::string desig = line.substr(45, 13);
                std::string iauo = line.substr(59, 21);

                int id = std::stoi(numid);

                bool is_valid = false;
                for (auto &c : name)
                    if (c != ' ')
                        is_valid = true;

                if (is_valid && name.find("simulation") == std::string::npos)
                    vv.push_back({id, name});
            }
            catch (std::exception &)
            {
            }
        }

        if (vv.size() > 0)
            return vv;
        else
            return horizonsoptions;
    }
}
