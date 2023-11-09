#include "tracking_widget.h"
#include "common/tracking/tle.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "core/module.h"
#include "core/config.h"
#include "main_ui.h"

#include "rotcl_handler.h"

namespace satdump
{
    TrackingWidget::TrackingWidget()
    {
        try
        {
            qth_lon = config::main_cfg["satdump_general"]["qth_lon"]["value"].get<double>();
            qth_lat = config::main_cfg["satdump_general"]["qth_lat"]["value"].get<double>();
            qth_alt = config::main_cfg["satdump_general"]["qth_alt"]["value"].get<double>();
        }
        catch (std::exception &e)
        {
            logger->error("Could not get QTH lon/lat! %s", e.what());
        }

        logger->trace("Using QTH %f %f Alt %f", qth_lon, qth_lat, qth_alt);

        if (general_tle_registry.size() > 0)
            has_tle = true;

        for (auto &tle : general_tle_registry)
            satoptions.push_back(tle.name);

        observer_station = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt);

        // Updates on registry updates
        eventBus->register_handler<TLEsUpdatedEvent>([this](TLEsUpdatedEvent)
                                                     {
                                                            tle_update_mutex.lock();

                                                            if (general_tle_registry.size() > 0)
                                                                has_tle = true;

                                                            satoptions.clear();
                                                            for (auto &tle : general_tle_registry)
                                                                satoptions.push_back(tle.name);
                                                                
                                                            tle_update_mutex.unlock(); });

        rotator_handler = std::make_shared<RotctlHandler>();

        // Restore settings
        loadConfig();

        // Start threads
        backend_thread = std::thread(&TrackingWidget::backend_run, this);
        rotatorth_thread = std::thread(&TrackingWidget::rotatorth_run, this);

        // Attempt to apply provided CLI settings
        if (satdump::config::main_cfg.contains("cli"))
        {
            auto &cli_settings = satdump::config::main_cfg["cli"];

            if (cli_settings.contains("engage_autotrack") && cli_settings["engage_autotrack"].get<bool>())
            {
                upcoming_satellite_passes_mtx.lock();
                autotrack_engaged = true;
                updateAutotrackPasses();

                if (upcoming_satellite_passes_sel.size() > 0)
                {
                    for (int i = 0; i < (int)general_tle_registry.size(); i++)
                        if (general_tle_registry[i].norad == upcoming_satellite_passes_sel[0].norad)
                            current_satellite = i;
                    horizons_mode = false;
                    backend_needs_update = true;
                    autotrack_pass_has_started = false;
                }
                else
                {
                    autotrack_engaged = false;
                }
                upcoming_satellite_passes_mtx.unlock();
            }
        }
    }

    TrackingWidget::~TrackingWidget()
    {
        predict_destroy_observer(observer_station);

        backend_should_run = false;
        if (backend_thread.joinable())
            backend_thread.join();

        rotatorth_should_run = false;
        if (rotatorth_thread.joinable())
            rotatorth_thread.join();
    }

    void TrackingWidget::render()
    {
        if (!has_tle)
            return;

        renderPolarPlot();
        ImGui::Separator();
        renderSelectionMenu();
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Object Information"))
            renderObjectStatus();
        if (ImGui::CollapsingHeader("Rotator Configuration"))
            renderRotatorStatus();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("Schedule and Config"))
            config_window_was_asked = show_window_config = true;

        renderConfigWindow();
    }

    double TrackingWidget::getTime()
    {
        auto time = std::chrono::system_clock::now();
        auto since_epoch = time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
        return millis.count() / 1e3;
    }

}
