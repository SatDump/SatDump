#include "scheduler.h"
#include "logger.h"
#include "core/plugin.h"
#include "common/utils.h"
#include "utils/time.h"

namespace satdump
{
    AutoTrackScheduler::AutoTrackScheduler()
    {
        auto tle_registry = general_tle_registry;
        if (tle_registry->size() > 0)
            has_tle = true;

        for (auto &tle : *tle_registry)
            satoptions.push_back(tle.name);

        // Updates on registry updates
        eventBus->register_handler<TLEsUpdatedEvent>([this](TLEsUpdatedEvent)
                                                     {
                                                            upcoming_satellite_passes_mtx.lock();

                                                            auto tle_registry = general_tle_registry;
                                                            if (tle_registry->size() > 0)
                                                                has_tle = true;

                                                            satoptions.clear();
                                                            for (auto &tle : *tle_registry)
                                                                satoptions.push_back(tle.name);
                                                                
                                                            upcoming_satellite_passes_mtx.unlock(); });
    }

    AutoTrackScheduler::~AutoTrackScheduler()
    {
        if (backend_should_run)
        {
            backend_should_run = false;
            if (backend_thread.joinable())
                backend_thread.join();
        }
    }

    void AutoTrackScheduler::processAutotrack(double curr_time)
    {
        if (autotrack_engaged)
        {
            upcoming_satellite_passes_mtx.lock();

            if (autotrack_cfg.multi_mode)
            {
                bool update = false;

                for (int i = 0; i < (int)upcoming_satellite_passes_sel.size(); i++)
                {
                    auto &pass = upcoming_satellite_passes_sel[i];
                    if (curr_time > pass.aos_time)
                    {
                        if (vfo_mode_norads_vis.count(pass.norad) == 0)
                        {
                            vfo_mode_norads_vis.insert({pass.norad, pass});

                            logger->critical("AOS!!!!!!!!!!!!!! %d", pass.norad);
                            TrackedObject obj;
                            for (auto &v : enabled_satellites)
                                if (v.norad == pass.norad)
                                    obj = v;
                            aos_callback(autotrack_cfg, pass, obj);
                        }
                    }
                }

            recheck:
                for (auto &p : vfo_mode_norads_vis)
                {
                    if (curr_time > p.second.los_time)
                    {
                        logger->critical("LOS!!!!!!!!!!!!!! %d ", p.first);
                        TrackedObject obj;
                        for (auto &v : enabled_satellites)
                            if (v.norad == p.first)
                                obj = v;
                        los_callback(autotrack_cfg, p.second, obj);

                        vfo_mode_norads_vis.erase(p.first);
                        update = true;
                        goto recheck;
                    }
                }

                if (update)
                {
                    updateAutotrackPasses(curr_time);
                    TrackedObject obj;
                    for (auto &v : enabled_satellites)
                        if (v.norad == upcoming_satellite_passes_sel[0].norad)
                            obj = v;
                    eng_callback(autotrack_cfg, upcoming_satellite_passes_sel[0], obj);
                }
            }
            else
            {
                if (upcoming_satellite_passes_sel.size() > 0)
                {
                    if (!autotrack_pass_has_started && curr_time > upcoming_satellite_passes_sel[0].aos_time)
                    {
                        logger->critical("AOS!!!!!!!!!!!!!! %d", upcoming_satellite_passes_sel[0].norad);
                        TrackedObject obj;
                        for (auto &v : enabled_satellites)
                            if (v.norad == upcoming_satellite_passes_sel[0].norad)
                                obj = v;
                        aos_callback(autotrack_cfg, upcoming_satellite_passes_sel[0], obj);
                        autotrack_pass_has_started = true;
                    }
                }

                if (curr_time > upcoming_satellite_passes_sel[0].los_time && upcoming_satellite_passes_sel.size() > 0)
                {
                    if (autotrack_pass_has_started)
                    {
                        logger->critical("LOS!!!!!!!!!!!!!! %d", upcoming_satellite_passes_sel[0].norad);
                        TrackedObject obj;
                        for (auto &v : enabled_satellites)
                            if (v.norad == upcoming_satellite_passes_sel[0].norad)
                                obj = v;
                        los_callback(autotrack_cfg, upcoming_satellite_passes_sel[0], obj);
                    }
                    autotrack_pass_has_started = false;
                    updateAutotrackPasses(curr_time);

                    {
                        TrackedObject obj;
                        for (auto &v : enabled_satellites)
                            if (v.norad == upcoming_satellite_passes_sel[0].norad)
                                obj = v;
                        eng_callback(autotrack_cfg, upcoming_satellite_passes_sel[0], obj);
                    }
                }
            }

            upcoming_satellite_passes_mtx.unlock();
        }
    }

    void AutoTrackScheduler::updateAutotrackPasses(double curr_time)
    {
        upcoming_satellite_passes_all.clear();

        for (TrackedObject &obj : enabled_satellites)
        {
#if 0
            if (obj.norad == 41105)
            {
                std::vector<satdump::SatellitePass> dump_passes;
                for (uint64_t ctime = curr_time; ctime < curr_time + 12 * 3600; ctime++)
                {
                    if ((ctime - 9 * 60) % (30 * 60) == 0)
                    {
                        dump_passes.push_back({0, ctime - 600, ctime, 0});
                    }
                }

                auto passes = getPassesForSatellite(41105, curr_time, 0, qth_lon, qth_lat, qth_alt, dump_passes);
                upcoming_satellite_passes_all.insert(upcoming_satellite_passes_all.end(), passes.begin(), passes.end());
            }
            else
#endif
            {
                auto passes = getPassesForSatellite(obj.norad, curr_time, 12 * 3600, qth_lon, qth_lat, qth_alt);
                for (auto &pass : passes)
                    if (pass.max_elevation > obj.min_elevation)
                        upcoming_satellite_passes_all.push_back(pass);
            }
        }

        upcoming_satellite_passes_all = filterPassesByElevation(upcoming_satellite_passes_all, autotrack_cfg.autotrack_min_elevation, 90); // TODO

        std::sort(upcoming_satellite_passes_all.begin(), upcoming_satellite_passes_all.end(),
                  [](SatellitePass &el1,
                     SatellitePass &el2)
                  {
                      return el1.aos_time < el2.aos_time;
                  });

        upcoming_satellite_passes_sel.clear();

        if (autotrack_cfg.multi_mode)
            upcoming_satellite_passes_sel = upcoming_satellite_passes_all;
        else
            upcoming_satellite_passes_sel = selectPassesForAutotrack(upcoming_satellite_passes_all);

        // for (auto ppp : upcoming_satellite_passes_sel)
        // logger->debug("Pass of %s at AOS %s LOS %s elevation %.2f",
        //               general_tle_registry->get_from_norad(ppp.norad).value().name.c_str(),
        //               timestamp_to_string(ppp.aos_time).c_str(),
        //               timestamp_to_string(ppp.los_time).c_str(),
        //               ppp.max_elevation);
    }

    void AutoTrackScheduler::setQTH(double qth_lon, double qth_lat, double qth_alt)
    {
        upcoming_satellite_passes_mtx.lock();
        this->qth_lon = qth_lon;
        this->qth_lat = qth_lat;
        this->qth_alt = qth_alt;
        // backend_needs_update = true;
        upcoming_satellite_passes_mtx.unlock();
    }

    void AutoTrackScheduler::setEngaged(bool v, double curr_time)
    {
        upcoming_satellite_passes_mtx.lock();
        autotrack_engaged = v;
        updateAutotrackPasses(curr_time);

        if (autotrack_engaged && upcoming_satellite_passes_sel.size() > 0)
        {
            TrackedObject obj;
            for (auto &v : enabled_satellites)
                if (v.norad == upcoming_satellite_passes_sel[0].norad)
                    obj = v;
            eng_callback(autotrack_cfg, upcoming_satellite_passes_sel[0], obj);
            autotrack_pass_has_started = false;
        }
        else
        {
            autotrack_engaged = false;
        }

        upcoming_satellite_passes_mtx.unlock();
    }

    bool AutoTrackScheduler::getEngaged()
    {
        return autotrack_engaged;
    }

    std::vector<TrackedObject> AutoTrackScheduler::getTracked()
    {
        return enabled_satellites;
    }

    void AutoTrackScheduler::setTracked(std::vector<TrackedObject> tracked)
    {
        upcoming_satellite_passes_mtx.lock();
        enabled_satellites = tracked;
        upcoming_satellite_passes_mtx.unlock();
    }

    void AutoTrackScheduler::start()
    {
        backend_should_run = true;
        backend_thread = std::thread(&AutoTrackScheduler::backend_run, this);
    }

    void AutoTrackScheduler::backend_run()
    {
        while (backend_should_run)
        {
            processAutotrack(getTime());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    AutoTrackCfg AutoTrackScheduler::getAutoTrackCfg()
    {
        return autotrack_cfg;
    }

    void AutoTrackScheduler::setAutoTrackCfg(AutoTrackCfg v)
    {
        upcoming_satellite_passes_mtx.lock();
        autotrack_cfg = v;
        upcoming_satellite_passes_mtx.unlock();
    }
}