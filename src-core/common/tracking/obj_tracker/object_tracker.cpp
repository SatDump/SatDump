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
        backend_should_run = false;
        if (backend_thread.joinable())
            backend_thread.join();

        rotatorth_should_run = false;
        if (rotatorth_thread.joinable())
            rotatorth_thread.join();

        predict_destroy_observer(satellite_observer_station);

        if (satellite_object != nullptr)
            predict_destroy_orbital_elements(satellite_object);
    }

    void ObjectTracker::setQTH(double qth_lon, double qth_lat, double qth_alt)
    {
        general_mutex.lock();
        this->qth_lon = qth_lon;
        this->qth_lat = qth_lat;
        this->qth_alt = qth_alt;
        if (satellite_observer_station != nullptr)
            predict_destroy_observer(satellite_observer_station);
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
        az_limits = rot->azLimits;
        el_limits = rot->elLimits;
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
