#include "tle_handler.h"
#include "core/config.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include <string>

namespace satdump
{
    TleDBHandler::TleDBHandler(std::shared_ptr<DBHandler> h) : DBHandlerBase(h) {}

    void TleDBHandler::init()
    {
        // Create TLE Table
        std::string sql_create_tle = "CREATE TABLE IF NOT EXISTS tle("
                                     "norad INT PRIMARY KEY NOT NULL,"
                                     "time INT NOT NULL,"
                                     "name TEXT NOT NULL,"
                                     "line1 TEXT NOT NULL,"
                                     "line2 TEXT NOT NULL);";
        if (h->run_sql(sql_create_tle))
            throw satdump_exception("Failed creating TLE database!");

        // Start auto-update (or update now?)
        autoUpdateTLE();

        all = get_all_tles(); // TODOREWORK REMOVE
    }

    void TleDBHandler::autoUpdateTLE()
    {
        std::string update_setting = satdump_cfg.getValueFromSatDumpGeneral<std::string>("tle_update_interval");
        time_t last_update = std::stoull(h->get_meta("tles_last_updated", "0"));
        bool honor_setting = true;
        time_t update_interval;

        if (update_setting == "Never")
            honor_setting = false;
        else if (update_setting == "4 hours")
            update_interval = 14400;
        else if (update_setting == "1 day")
            update_interval = 86400;
        else if (update_setting == "3 days")
            update_interval = 259200;
        else if (update_setting == "7 days")
            update_interval = 604800;
        else if (update_setting == "14 days")
            update_interval = 1209600;
        else
        {
            logger->error("Invalid TLE Auto-update interval: %s", update_setting.c_str());
            honor_setting = false;
        }

        // Update now, if needed
        time_t now = time(NULL);
        if (/*(honor_setting && now > last_update + update_interval) ||*/ h->get_table_size("tle") <= 0)
        {
            updateTLEDatabase();
            last_update = now;
        }

        // Schedule updates while running
        if (honor_setting)
        {
            eventBus->register_handler<AutoUpdateTLEsEvent>([this](AutoUpdateTLEsEvent evt) { updateTLEDatabase(); });
            std::shared_ptr<AutoUpdateTLEsEvent> evt = std::make_shared<AutoUpdateTLEsEvent>();
            taskScheduler->add_task<AutoUpdateTLEsEvent>("auto_tle_update_todorework", evt, last_update, update_interval);
        }
    }

    void TleDBHandler::updateTLEDatabase()
    {
        logger->info("Updating TLEs...");
        std::vector<int> norads_to_fetch = satdump_cfg.main_cfg["tle_settings"]["tles_to_fetch"].get<std::vector<int>>();
        std::vector<std::string> urls_to_fetch = satdump_cfg.main_cfg["tle_settings"]["urls_to_fetch"].get<std::vector<std::string>>();

        for (auto &url_str : urls_to_fetch)
        {
            auto tles = tryFetchTLEsFromFileURL(url_str);

            h->tr_begin();
            for (auto &t : tles)
                putTLE(t);
            h->tr_end();
        }

        for (int norad : norads_to_fetch)
        {
            auto tles = tryFetchSingleTLEwithNorad(norad);
            if (tles.size() == 1)
                putTLE(tles[0]);
            else
                logger->error("There should only be one TLE per norad! %d", norad);
        }

        // Update last update timestamp & other stuff
        h->set_meta("tles_last_updated", std::to_string(time(0)));
        logger->info("%d TLEs in database!", h->get_table_size("tle"));
        eventBus->fire_event<TLEsUpdatedEvent>(TLEsUpdatedEvent());
        all = get_all_tles(); // TODOREWORK REMOVE
    }

    void TleDBHandler::putTLE(TLE tle)
    {
        std::string sql = "INSERT INTO tle (norad, name, time, line1, line2) VALUES (" + std::to_string(tle.norad) + //
                          ", \"" + tle.name + "\", '" + std::to_string(time(0)) + "', '" + tle.line1 + "', '" +      //
                          tle.line2 + "') ON CONFLICT(norad) DO UPDATE SET name=\"" + tle.name +                     //
                          "\", time='" + std::to_string(time(0)) + "', line1='" + tle.line1 + "', line2='" + tle.line2 + "';";

        char *err = NULL;
        if (sqlite3_exec(h->db, sql.c_str(), NULL, 0, &err))
        {
            logger->error("Error inserting TLE in database! %s", err);
            sqlite3_free(err);
        }
    }

    std::optional<TLE> TleDBHandler::get_from_norad(int norad)
    {
        TLE r;
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(h->db, ("SELECT name,line1,line2 from tle where norad=" + std::to_string(norad)).c_str(), -1, &res, 0))
            logger->error("Couldn't fetch TLE from DB! " + std::string(sqlite3_errmsg(h->db)));
        else if (sqlite3_step(res) == SQLITE_ROW)
        {
            r.norad = norad;
            r.name = (char *)sqlite3_column_text(res, 0);
            r.line1 = (char *)sqlite3_column_text(res, 1);
            r.line2 = (char *)sqlite3_column_text(res, 2);
        }

        sqlite3_finalize(res);
        return r;
    }

    std::optional<TLE> TleDBHandler::get_from_norad_time(int norad, time_t timestamp)
    {
        auto t = get_from_spacetrack_time(norad, timestamp);
        if (t.has_value())
            return t;
        else
            return get_from_norad(norad);
    }

    std::vector<TLE> TleDBHandler::get_all_tles()
    {
        std::vector<TLE> rr;
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(h->db, "select norad,name,line1,line2 from tle;", -1, &res, 0))
            logger->error("Couldn't fetch TLE from DB! " + std::string(sqlite3_errmsg(h->db)));
        else
        {
            while (sqlite3_step(res) == SQLITE_ROW)
            {
                TLE r;
                r.norad = sqlite3_column_int(res, 0);
                r.name = (char *)sqlite3_column_text(res, 1);
                r.line1 = (char *)sqlite3_column_text(res, 2);
                r.line2 = (char *)sqlite3_column_text(res, 3);
                rr.push_back(r);
            }
        }

        sqlite3_finalize(res);
        return rr;
    }
} // namespace satdump