#include "object_tracker.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/tracking/tle.h"
#include "core/plugin.h"
#include "common/utils.h"

namespace satdump
{
    ObjectTracker::ObjectTracker(bool is_gui) : is_gui(is_gui)
    {
        if (general_tle_registry.size() > 0)
            has_tle = true;

        for (auto &tle : general_tle_registry)
            satoptions.push_back(tle.name);

        satellite_observer_station = predict_create_observer("Main", 0, 0, 0);

        // Updates on registry updates
        eventBus->register_handler<TLEsUpdatedEvent>([this](TLEsUpdatedEvent)
                                                     {
                                                            general_mutex.lock();

                                                            if (general_tle_registry.size() > 0)
                                                                has_tle = true;

                                                            satoptions.clear();
                                                            for (auto &tle : general_tle_registry)
                                                                satoptions.push_back(tle.name);
                                                                
                                                            general_mutex.unlock(); });

        // Start threads
        backend_thread = std::thread(&ObjectTracker::backend_run, this);
        rotatorth_thread = std::thread(&ObjectTracker::rotatorth_run, this);
    }

    ObjectTracker::~ObjectTracker()
    {
        predict_destroy_observer(satellite_observer_station);

        backend_should_run = false;
        if (backend_thread.joinable())
            backend_thread.join();

        rotatorth_should_run = false;
        if (rotatorth_thread.joinable())
            rotatorth_thread.join();
    }

    nlohmann::json ObjectTracker::getStatus()
    {
        nlohmann::json v;

        std::string obj_name = "None";
        if (tracking_mode == TRACKING_HORIZONS)
            obj_name = horizonsoptions[current_horizons_id].second;
        else if (tracking_mode == TRACKING_SATELLITE)
            obj_name = satoptions[current_satellite_id];
        v["object_name"] = obj_name;
        v["sat_current_pos"] = sat_current_pos;

        if (tracking_mode == TRACKING_SATELLITE && satellite_object != nullptr)
        {
            v["sat_azimuth_rate"] = satellite_observation_pos.azimuth_rate * RAD_TO_DEG;
            v["sat_elevation_rate"] = satellite_observation_pos.elevation_rate * RAD_TO_DEG;
        }

        v["next_aos_time"] = next_aos_time;
        v["next_los_time"] = next_los_time;

        double timeOffset = 0, ctime = getTime();
        if (next_aos_time > ctime)
            timeOffset = next_aos_time - ctime;
        else
            timeOffset = next_los_time - ctime;

        v["next_event_in"] = timeOffset;

        v["rotator_engaged"] = rotator_engaged;
        v["rotator_tracking"] = rotator_tracking;
        v["rot_current_pos"] = rot_current_pos;
        v["rot_current_req_pos"] = rot_current_req_pos;

        return v;
    }

    void ObjectTracker::setQTH(double qth_lon, double qth_lat, double qth_alt)
    {
        general_mutex.lock();
        this->qth_lon = qth_lon;
        this->qth_lat = qth_lat;
        this->qth_alt = qth_alt;
        satellite_observer_station = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt);
        backend_needs_update = true;
        general_mutex.unlock();
    }

    void ObjectTracker::setObject(TrackingMode mode, int objid)
    {
        general_mutex.lock();
        tracking_mode = TRACKING_NONE;

        if (mode == TRACKING_HORIZONS)
        {
            if (horizonsoptions.size() == 1)
                horizonsoptions = pullHorizonsList();
            for (int i = 0; i < (int)horizonsoptions.size(); i++)
            {
                if (horizonsoptions[i].first == objid)
                {
                    tracking_mode = TRACKING_HORIZONS;
                    current_horizons_id = i;
                    break;
                }
            }
        }
        else if (mode == TRACKING_SATELLITE)
        {
            for (int i = 0; i < (int)satoptions.size(); i++)
            {
                if (general_tle_registry[i].norad == objid)
                {
                    tracking_mode = TRACKING_SATELLITE;
                    current_satellite_id = i;
                    break;
                }
            }
        }

        backend_needs_update = true;
        general_mutex.unlock();
    }

    void ObjectTracker::setRotator(std::shared_ptr<rotator::RotatorHandler> rot)
    {
        rotator_handler_mtx.lock();
        rotator_handler = rot;
        rotator_handler_mtx.unlock();
    }

    void ObjectTracker::setRotatorEngaged(bool v)
    {
        rotator_handler_mtx.lock();
        rotator_engaged = v;
        rotator_handler_mtx.unlock();
    }

    void ObjectTracker::setRotatorTracking(bool v)
    {
        rotator_handler_mtx.lock();
        rotator_tracking = v;
        rotator_handler_mtx.unlock();
    }

    void ObjectTracker::setRotatorReqPos(float az, float el)
    {
        rotator_handler_mtx.lock();
        rot_current_req_pos.az = az;
        rot_current_req_pos.el = el;
        rotator_handler_mtx.unlock();
    }
}
