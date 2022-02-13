#pragma once

#ifdef BUILD_TRACKING

#include <thread>
#include "common/geodetic/geodetic_coordinates.h"
#include "satellite_tracker.h"

class TrackingUI
{
private:
    unsigned int map_texture;
    std::thread tracking_thread;

    int qth_selection = 0;

    int passes_delta = 3600 * 24;

    bool showPanel = true;

    //int window_width;

    int liveSimulatedTimeOffset = 0;
    bool liveTime = true, simulatedLiveTime = false;
    double current_time;

    std::mutex eventMutex;
    std::vector<ObjectStatus> upcomingevents;

    std::vector<std::shared_ptr<GroundQTH>> qth_list;
    SatelliteTracker tracker;

    std::mutex azElMutex;

    bool showAddQth = false;
    char newqthname[1000];
    float newQthLat = 0;
    float newQthLon = 0;
    float newQthAlt = 0;

    void updateThread();

public:
    TrackingUI();
    ~TrackingUI();

    void toMapCoords(float lat, float lon, int &x, int &y);

    void draw(int wwidth, int wheight);
};

#endif