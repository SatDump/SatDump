#include "core/config.h"
#include "kepler_handler.h"
#include "libs/predict/predict.h"
#include "logger.h"
#include "utils/format.h"
#include "utils/http.h"
#include "utils/string.h"
#include <exception>
#include <thread>

#if defined(_WIN32)
#include <time.h>

#ifdef timegm
#undef timegm
#endif

inline time_t timegm(struct tm *const t) { return _mkgmtime(t); }
#endif

namespace satdump
{
    template <typename T>
    std::string strL(T val, int sz, bool leading_dec = false)
    {
        std::string str = to_string_with_precision(val, 30);

        if (str.size() > 2 && str[0] == '0' && str[1] == '.')
            str = leading_dec ? str.substr(2, str.size() - 2) : str.substr(1, str.size() - 1);

        if (str.size() > 3 && str[0] == '-' && str[1] == '0' && str[2] == '.')
            str = leading_dec ? ("-" + str.substr(3, str.size() - 3)) : ("-" + str.substr(2, str.size() - 2));

        if (str.size() > sz)
            str.resize(sz);
        if (str.size() < sz)
            while (str.size() < sz)
                str = " " + str;

        return str;
    }

    std::string strLE(double val, int sz)
    {
        if (val == 0)
        {
            std::string res(sz - 2, '0');
            return res + "-0";
        }
        else if (val < 1)
        {
            int mult = 0;
            while (fmod(val * pow(10, mult), 1) != 0)
                mult++;

            // logger->critical("%d %d", mult, mult - (sz - 3));

            int coef = mult - (sz - 3);

            int coefflen = std::to_string(coef).size();

            std::string res = std::to_string((int)(val * pow(10, mult)));

            if (res.size() > sz - 1 - coefflen)
                res.resize(sz - 1 - coefflen);
            if (res.size() < sz - 1 - coefflen)
                while (res.size() < sz - 1 - coefflen)
                    res = " " + res;

            res += "-";
            res += std::to_string(coef);

            return res;
        }
        else
            throw satdump_exception("Error, value must be below 1!");
    }

    TLE keplerToTle(KeplerData kep)
    {
        TLE tle;
        tle.name = kep.name;
        tle.norad = kep.satellite_number;
        tle.time = kep.epoch;

        int tle_id = kep.satellite_number > 99999 ? 99999 : kep.satellite_number;

        time_t tmptime = kep.epoch;
        tm timeS = *gmtime(&tmptime);
        int year = timeS.tm_year;
        // logger->critical(year);
        memset(&timeS, 0, sizeof(struct tm));
        timeS.tm_year = year;
        double yearT = timegm(&timeS);

        double days = (kep.epoch - yearT) / (3600.0 * 24.0);

        std::string des = kep.designator;
        if (des.size() == 9)
        {
            std::string ori = des;
            des.resize(8);

            for (auto &c : des)
                c = ' ';

            des[0] = ori[2];
            des[1] = ori[3];

            des[2] = ori[5];
            des[3] = ori[6];
            des[4] = ori[7];

            des[5] = ori[8];
        }
        else
            des.resize(8);

        tle.line1 = "1 " +                                              // Line Number
                    strL(tle_id, 5) +                                   // Satellite Number
                    "U " +                                              // Classification
                    des +                                               // International Designator
                    " " + strL(year - 100, 2) +                         // EPoch Year (last 2)
                    strL(days, 12) +                                    // Epoch DOY
                    " " + strL(kep.derivative_mean_motion, 10) +        // First derivative mean motion
                    " " + strLE(kep.second_derivative_mean_motion, 8) + // Second derivative mean motion
                    " " + strLE(kep.bstar_drag_term, 8) +               // Drag term
                    " 0 " +                                             // Ephemeris type
                    strL(kep.element_number, 4) +                       // Element set number
                    "C";                                                // Checksum

        tle.line2 = "2 " +                                   // Line Number
                    strL(tle_id, 5) +                        // Satellite Number
                    " " + strL(kep.inclination, 8) +         // Inclination
                    " " + strL(kep.right_ascension, 8) +     // Right Ascension
                    " " + strL(kep.eccentricity, 7, 1) +     // Eccentricity
                    " " + strL(kep.argument_of_perigee, 8) + // Arg of perigee
                    " " + strL(kep.mean_anomaly, 8) +        // Mean anomaly
                    " " + strL(kep.mean_motion, 11) +        // Mean motion
                    strL(kep.revolutions_at_epoch, 5) +      // Revs at epoch
                    "C";                                     // Checksum

        return tle;
    }

    bool ccsdsOmmToKepler(std::string omm, KeplerData &kep)
    {
        try
        {
            auto elems = splitString(omm, ',');

            if (elems.size() != 17)
                return false;

            std::tm timeS;
            double seconds;
            memset(&timeS, 0, sizeof(std::tm));
            if (sscanf(elems[2].c_str(), "%4d-%2d-%2dT%2d:%2d:%lf", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &seconds) == 6)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                kep.epoch = timegm(&timeS) + seconds;
            }

            kep.name = elems[0];
            kep.designator = elems[1];

            kep.mean_motion = std::stod(elems[3]);
            kep.eccentricity = std::stod(elems[4]);
            kep.inclination = std::stod(elems[5]);
            kep.right_ascension = std::stod(elems[6]);
            kep.argument_of_perigee = std::stod(elems[7]);
            kep.mean_anomaly = std::stod(elems[8]);

            kep.satellite_number = std::stod(elems[11]);
            kep.element_number = std::stod(elems[12]);
            kep.revolutions_at_epoch = std::stod(elems[13]);

            kep.bstar_drag_term = std::stod(elems[14]);
            kep.derivative_mean_motion = std::stod(elems[15]);
            kep.second_derivative_mean_motion = std::stod(elems[16]);
        }
        catch (std::exception &e)
        {
            return false;
        }

        return true;
    }

    bool tleToKepler(TLE tle, KeplerData &kep)
    {
        auto tle_ephems = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());

        if (!tle_ephems)
            return false;

        kep.satellite_number = tle_ephems->satellite_number; // Satellite number
        kep.element_number = tle_ephems->element_number;     // Element number
        kep.name = tle.name;                                 // Satellite Name
        kep.designator = tle_ephems->designator;             // International designator

        std::tm timeS;
        memset(&timeS, 0, sizeof(std::tm));
        timeS.tm_year = (2000 + tle_ephems->epoch_year) - 1900;
        kep.epoch = timegm(&timeS) + tle_ephems->epoch_day * (3600. * 24.);

        kep.inclination = tle_ephems->inclination;                                     // Inclination
        kep.right_ascension = tle_ephems->right_ascension;                             // Right Ascension of the Ascending Node [Degrees]
        kep.eccentricity = tle_ephems->eccentricity;                                   // Eccentricity
        kep.argument_of_perigee = tle_ephems->argument_of_perigee;                     // Argument of Perigee [Degrees]
        kep.mean_anomaly = tle_ephems->mean_anomaly;                                   // Mean Anomaly [Degrees]
        kep.mean_motion = tle_ephems->mean_motion;                                     // Mean Motion [Revs per day]
        kep.derivative_mean_motion = tle_ephems->derivative_mean_motion;               // First Time Derivative of the Mean Motion divided by two
        kep.second_derivative_mean_motion = tle_ephems->second_derivative_mean_motion; // Second Time Derivative of Mean Motion divided by six
        kep.bstar_drag_term = tle_ephems->bstar_drag_term;                             // BSTAR drag term
        kep.revolutions_at_epoch = tle_ephems->revolutions_at_epoch;                   // Number of revolutions around Earth at epoch

        predict_destroy_orbital_elements(tle_ephems);

        return true;
    }

    std::vector<KeplerData> parseCcsdsOmmFile(std::string fileCont)
    {
        std::istringstream omm_stream(fileCont);

        std::vector<KeplerData> keps;

        std::string this_line;
        while (std::getline(omm_stream, this_line))
        {
            KeplerData kep;
            if (ccsdsOmmToKepler(this_line, kep))
                keps.push_back(kep);
        }

        return keps;
    }

    std::vector<KeplerData> tryFetchOMMFileFromURL(std::string url_str)
    {
        bool success = true;
        std::vector<KeplerData> new_registry;

        logger->info(url_str);
        std::string result;
        int http_res = 1, trials = 0;
        while (http_res == 1 && trials < 10)
        {
            if ((http_res = perform_http_request(url_str, result)) != 1)
            {
                new_registry = parseCcsdsOmmFile(result);
                success = new_registry.size() > 0;
            }
            else
                success = false;
            trials++;
            if (!success)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                logger->info("Failed getting OMMs. Retrying...");
            }
        }

        if (!success)
            logger->warn("Failed to get OMM for %s", url_str.c_str());

        return new_registry;
    }

    std::vector<KeplerData> tryFetchSingleOMMwithNorad(int norad)
    {
        bool success = true;
        std::vector<KeplerData> new_registry;

        std::string url_str = satdump_cfg.main_cfg["kepler_settings"]["url_template"].get<std::string>();
        while (url_str.find("%NORAD%") != std::string::npos)
            url_str.replace(url_str.find("%NORAD%"), 7, std::to_string(norad));

        logger->info(url_str);
        std::string result;
        int http_res = 1, trials = 0;
        while (http_res == 1 && trials < 10)
        {
            if ((http_res = perform_http_request(url_str, result)) != 1)
            {
                new_registry = parseCcsdsOmmFile(result);
                success = new_registry.size() > 0;
            }
            else
                success = false;
            trials++;
            if (!success)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                logger->info("Failed getting Kepler. Retrying...");
            }
        }

        if (!success)
            logger->error("Error updating Kepler for %d. Ignoring!", norad);

        return new_registry;
    }
} // namespace satdump