#pragma once

#ifdef BUILD_TRACKING

#include "common/geodetic/geodetic_coordinates.h"
#include "libs/predict/predict.h"
#include <vector>
#include "nlohmann/json.hpp"

class TrackingUI;

struct AzEl
{
    float az;
    float el;
};

// Class holding the information about a specific object
// This is meant as a semi-opaque struct, exposing some common
// information but also holding internal stuff that will
// only work with the right class
struct ObjectStruct
{
    bool isEnabled;                                             // If the object is enabled
    bool isSelected;                                            // If the object is selected
    std::string name;                                           // Object name to display
    geodetic::geodetic_coords_t position;                       // Current geodetic position
    std::vector<geodetic::geodetic_coords_t> upcomingPositions; // Upcoming object positions, eg, upcoming orbit
};

struct ObjectStatus
{
    ObjectStruct *obj;         // Object struct
    double timeUntilNextEvent; // Time in seconds before next AOS or LOS. -1 = Never
    bool isVisible;            // If the object is visible
    AzEl azel;                 // Current Az/El
};

struct ObjectPass
{
    ObjectStruct *obj;

    double aos_time;
    AzEl aos_azel;
    double tca_time;
    AzEl tca_azel;
    double los_time;
    AzEl los_azel;

    std::vector<AzEl> passCurve;
};

class GroundQTH
{
private:
    geodetic::geodetic_coords_t location;
    predict_observer_t *predict_observer;
    std::string name;

public:
    bool showMenu;

    GroundQTH(geodetic::geodetic_coords_t location, std::string name) : location(location), name(name)
    {
        location.toRads();
        predict_observer = predict_create_observer("QTH", location.lat, location.lon, location.alt);
    }
    ~GroundQTH()
    {
    }

    predict_observer_t *get_predict() { return predict_observer; }

    std::string getName() { return name; }
    geodetic::geodetic_coords_t getPosition() { return location; }

    std::vector<ObjectPass> upcomingPasses;

    nlohmann::json serialize()
    {
        location.toDegs();
        nlohmann::json json;
        json["name"] = name;
        json["lat"] = location.lat;
        json["lon"] = location.lon;
        json["alt"] = location.alt;
        return json;
    }

    GroundQTH(nlohmann::json json)
    {
        location.toDegs();

        name = json["name"].get<std::string>();
        location.lat = json["lat"].get<double>();
        location.lon = json["lon"].get<double>();
        location.alt = json["alt"].get<double>();

        location.toRads();
        predict_observer = predict_create_observer("QTH", location.lat, location.lon, location.alt);
        location.toDegs();
    }
};

/*
 ObjectTracker 
*/
class ObjectTracker
{
protected:
    double current_time; // Current time of the entire tracker

public:
    double getCurrentTime() { return current_time; }                                                                               // Get current tracker time
    virtual void updateToTime(double time) = 0;                                                                                    // Update all internals to this time
    virtual void drawMenu() = 0;                                                                                                   // Draw menu (to be in the left-side panel)
    virtual void drawWindows() = 0;                                                                                                // Draw other windows, for object information or whatever else
    virtual void drawElementsOnMap(bool isHovered, bool leftClicked, bool rightClicked, TrackingUI &ui, ObjectStruct *object) = 0; // Draw additional object stuff, eg, satellite footprint, orbit, etc
    virtual AzEl getAzElForObject(ObjectStruct *object, GroundQTH *qth) = 0;                                                       // Get current Az/El for a specific object
    virtual std::vector<ObjectStatus> getObjectStatuses(GroundQTH *qth) = 0;                                                       // Get object status(es)
    virtual std::vector<AzEl> getUpcomingAzElForObject(ObjectStruct *object, GroundQTH *qth) = 0;
    virtual std::vector<ObjectPass> getUpcomingPassForObjectAtQTH(ObjectStruct *object, GroundQTH *qth, int timeEnd) = 0;
};

#endif