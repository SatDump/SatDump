#pragma once

#ifdef BUILD_TRACKING

#include "object_tracker.h"
#include <mutex>
#include "tle.h"
#include <atomic>
#include <vector>
#include "libs/predict/predict.h"
#include <memory>
#include <map>
#include <vector>
#include <algorithm>
#include "nlohmann/json.hpp"

#include "utils.h"

struct SatelliteObject : public ObjectStruct
{
    //std::mutex positionMutex;
    double currentTime;

    bool ui_needs_update;

    tle::TLE tle;
    predict_orbital_elements_t *orbital_elements;
    predict_position satellite_position;
    double orbit_period;

    bool drawMore = false, showMenu = false;

    bool showUImenu = false;

    int trace_r, trace_g, trace_b;

    SatelliteObject(int norad)
    {
        isEnabled = false;
        tle = tle::getTLEfromNORAD(norad); // This can be safely harcoded, only 1 satellite
        name = tle.name;
        orbital_elements = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());
        orbit_period = double(3600 * 24) / orbital_elements->mean_motion;

        trace_r = rand() % 255;
        trace_g = rand() % 255;
        trace_b = rand() % 255;
    }
    ~SatelliteObject()
    {
        predict_destroy_orbital_elements(orbital_elements);
    }

    std::mutex footprintMutex;
    geodetic::geodetic_coords_t footprint_points[360];
    void computeFootprint()
    {
        footprintMutex.lock();

        /* Range circle calculations.
        * Borrowed from GPredict by Aang23 who borrowed from 
        * gsat 0.9.0 by Xavier Crehueras, EB3CZS
        * who borrowed from John Magliacane, KD2BD.
        * Optimized by Alexandru Csete and William J Beksi.
        */
        geodetic::geodetic_coords_t pos = position;

        double ssplat = pos.toRads().lat; // * de2ra;
        double ssplon = pos.toRads().lon;
        double beta = (0.5 * satellite_position.footprint) / xkmper;

        double rangelon;

        for (double azi = 0; azi < 180; azi += 1)
        {
            double azimuth = DEG_TO_RAD * (double)azi;
            double rangelat = asin(sin(ssplat) * cos(beta) + cos(azimuth) * sin(beta) * cos(ssplat));
            double num = cos(beta) - (sin(ssplat) * sin(rangelat));
            double dem = cos(ssplat) * cos(rangelat);

            if (azi == 0 && north_pole_is_covered(pos, satellite_position.footprint))
                rangelon = ssplon + M_PI;
            else if (fabs(num / dem) > 1.0)
                rangelon = ssplon;
            else
            {
                if ((180.0 - azi) >= 0)
                    rangelon = ssplon - arccos(num, dem);
                else
                    rangelon = ssplon + arccos(num, dem);
            }

            rangelat = rangelat * RAD_TO_DEG;
            rangelon = rangelon * RAD_TO_DEG;

            //toMapCoords(rangelat, rangelon, x, y);

            footprint_points[int(azi)] = geodetic::geodetic_coords_t(rangelat, rangelon, 0);

            double lon2;
            mirror_lon(position, rangelon, lon2, 180);
            //toMapCoords(rangelat, lon2, x, y);
            footprint_points[int(359 - azi)] = geodetic::geodetic_coords_t(rangelat, lon2, 0);
        }

        footprintMutex.unlock();
    }

    std::mutex upcomingMutex;
    std::vector<geodetic::geodetic_coords_t> upcomingOrbit;
    void updateUpcoming()
    {
        upcomingMutex.lock();
        upcomingOrbit.clear();
        for (int timeOffset = 0; timeOffset < orbit_period * 2; timeOffset += (orbit_period / 500))
            upcomingOrbit.push_back(getPositionAt(currentTime + timeOffset));
        upcomingMutex.unlock();
    }

    geodetic::geodetic_coords_t getPositionAt(double time)
    {
        predict_position satellite_pos;
        predict_orbit(orbital_elements, &satellite_pos, predict_to_julian_double(time));
        return geodetic::geodetic_coords_t(satellite_pos.latitude, satellite_pos.longitude, satellite_pos.altitude, true).toDegs();
    }

    AzEl getAzElAt(double time, GroundQTH *qth)
    {
        predict_position satellite_pos;
        predict_orbit(orbital_elements, &satellite_pos, predict_to_julian_double(time));
        predict_observation observation;
        predict_observe_orbit(qth->get_predict(), &satellite_pos, &observation);
        return {float(observation.azimuth * RAD_TO_DEG), float(observation.elevation * RAD_TO_DEG)};
    }

    std::map<std::string, std::vector<AzEl>> upcomingAzEl;
    void updateUpcomingAzEl(GroundQTH *qth)
    {
        if (upcomingAzEl.count(qth->getName()) <= 0)
            upcomingAzEl.emplace(qth->getName(), std::vector<AzEl>());

        std::vector<AzEl> &upcoming = upcomingAzEl[qth->getName()];
        upcoming.clear();

        predict_observation next_los = predict_next_los(qth->get_predict(), orbital_elements, predict_to_julian(currentTime));
        double los = predict_from_julian(next_los.time);

        for (int timeOffset = currentTime; timeOffset < los; timeOffset += 10)
            if (getAzElAt(timeOffset, qth).el > 0)
                upcoming.push_back(getAzElAt(timeOffset, qth));
    }
};

struct SatelliteGroup
{
    std::string name;
    std::vector<SatelliteGroup> subgroups;
    std::vector<int> satellites;
    bool isEnabled = true;

    bool hasSatellite(int norad)
    {
        bool has = false;

        if (std::find(satellites.begin(), satellites.end(), norad) != satellites.end())
            has = has || true;

        for (SatelliteGroup &group : subgroups)
            has = has || group.hasSatellite(norad);

        return has;
    }

    bool hasGroup(std::string name)
    {
        bool has = false;

        if (name == this->name)
            has = has || true;

        for (SatelliteGroup &group : subgroups)
            has = has || group.hasGroup(name);

        return has;
    }

    void removeGroup(std::string name)
    {
        const std::vector<SatelliteGroup>::iterator it = std::find_if(subgroups.begin(), subgroups.end(),
                                                                      [&name](const SatelliteGroup &m) -> bool
                                                                      { return m.name == name; });

        if (it != subgroups.end())
        {
            subgroups.erase(it);
        }
        else
        {
            for (SatelliteGroup &group : subgroups)
                group.removeGroup(name);
        }
    }

    nlohmann::json serialize()
    {
        nlohmann::json json;

        json["name"] = name;
        json["satellites"] = satellites;
        json["enabled"] = isEnabled;

        for (int i = 0; i < subgroups.size(); i++)
        {
            if (subgroups[i].name != "Unsorted")
                json["subgroups"][i] = subgroups[i].serialize();
        }

        return json;
    }

    void deserialize(nlohmann::json json)
    {
        name = json["name"].get<std::string>();
        satellites = json["satellites"].get<std::vector<int>>();
        isEnabled = json["enabled"].get<bool>();

        for (int i = 0; i < json["subgroups"].size(); i++)
        {
            SatelliteGroup group;
            group.deserialize(json["subgroups"][i]);
            subgroups.push_back(group);
        }
    }
};

struct DragDropSat
{
    int norad;
    SatelliteGroup *group;
};

struct DragDropGroup
{
    SatelliteGroup *group;
};

class SatelliteTracker
{
public:
    std::mutex satellites_mutex;
    std::vector<std::shared_ptr<SatelliteObject>> satellites;
    bool ui_needs_update;
    bool showConfig = false;

    bool newGroupDialog = false;
    char newgroupname[1000];

    bool newSatDialog = false;
    int newNorad = 0;

    SatelliteGroup mainGroup = {"MAIN"};

    void displayGroup(SatelliteGroup &group, bool clicked = false, bool status = false);
    void updateUnsorted();

public:
    SatelliteTracker();
    ~SatelliteTracker();
    virtual void updateToTime(double time);
    virtual void drawMenu();
    virtual void drawWindows();
    virtual void drawElementsOnMap(bool isHovered, bool leftClicked, bool rightClicked, TrackingUI &ui, ObjectStruct *object);
    virtual AzEl getAzElForObject(ObjectStruct *object, GroundQTH *qth);
    virtual void updateUpcomingAzElForObject(ObjectStruct *object, GroundQTH *qth);
    virtual std::vector<AzEl> getUpcomingAzElForObject(ObjectStruct *object, GroundQTH *qth);
    virtual std::vector<ObjectStatus> getObjectStatuses(GroundQTH *qth);
    virtual std::vector<ObjectPass> getUpcomingPassForObjectAtQTH(ObjectStruct *object, GroundQTH *qth, double timeEnd);
};

#endif