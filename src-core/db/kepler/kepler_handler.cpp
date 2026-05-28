#include "kepler_handler.h"
#include "core/config.h"
#include "db/db_handler.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "utils/string.h"
#include <string>

namespace satdump
{
    KeplerDBHandler::KeplerDBHandler(std::shared_ptr<DBHandler> h) : DBHandlerBase(h) {}

    void KeplerDBHandler::init()
    {
        // Create Kepler Table
        std::string sql_create_kep = "CREATE TABLE IF NOT EXISTS kepler("
                                     "satellite_number INT NOT NULL,"
                                     "element_number INT NOT NULL,"
                                     "name TEXT NOT NULL,"
                                     "designator TEXT NOT NULL,"
                                     "epoch REAL NOT NULL,"
                                     "inclination REAL NOT NULL,"
                                     "right_ascension REAL NOT NULL,"
                                     "eccentricity REAL NOT NULL,"
                                     "argument_of_perigee REAL NOT NULL,"
                                     "mean_anomaly REAL NOT NULL,"
                                     "mean_motion REAL NOT NULL,"
                                     "derivative_mean_motion REAL NOT NULL,"
                                     "second_derivative_mean_motion REAL NOT NULL,"
                                     "bstar_drag_term REAL NOT NULL,"
                                     "revolutions_at_epoch INT NOT NULL);";
        if (h->run_sql(sql_create_kep))
            throw satdump_exception("Failed creating Kepler database!");

        // Start auto-update (or update now?)
        // autoUpdateTLE();

        // all = get_all_tles(); // TODOREWORK REMOVE
    }

#if 0
    void KeplerDBHandler::autoUpdateTLE()
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

#if 0
        // Update now, if needed
        time_t now = time(NULL);
        if (/*(honor_setting && now > last_update + update_interval) ||*/ h->get_table_size("tle") <= 0)
        {
            updateTLEDatabase();
            last_update = now;
        }
#endif

        // Schedule updates while running
        if (honor_setting)
        {
            eventBus->register_handler<AutoUpdateTLEsEvent>([this](AutoUpdateTLEsEvent evt) { updateTLEDatabase(); });
            std::shared_ptr<AutoUpdateTLEsEvent> evt = std::make_shared<AutoUpdateTLEsEvent>();
            taskScheduler->add_task<AutoUpdateTLEsEvent>("auto_tle_update_todorework", evt, last_update, update_interval);
        }
    }

    void KeplerDBHandler::updateTLEDatabase()
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

        {
            time_t tt = time(0);
            std::vector<int> norads;
            for (auto &t : get_all_tles())
                if (tt - t.time > (3600 * 24 * 2)) // TODOREWORK respect update interval!
                    norads.push_back(t.norad);

            if (norads.size())
                logger->error("%d TLEs are too old in database, even after attempting an update. Pulling from space-track. This is NOT optimal!", norads.size());

            while (norads.size() > 0)
            {
                std::vector<int> cnorads = norads;
                cnorads.resize(std::min<int>(2000, cnorads.size()));
                norads.erase(norads.begin(), norads.begin() + cnorads.size());
                auto tles = get_from_spacetrack_latest_list(cnorads); // tryFetchTLEsFromFileURL(url_str);

                h->tr_begin();
                for (auto &t : tles)
                    putTLE(t);
                h->tr_end();
            }
        }

        // Update last update timestamp & other stuff
        h->set_meta("tles_last_updated", std::to_string(time(0)));
        logger->info("%d TLEs in database!", h->get_table_size("tle"));
        eventBus->fire_event<TLEsUpdatedEvent>(TLEsUpdatedEvent());
        all = get_all_tles(); // TODOREWORK REMOVE
    }
#endif

    void KeplerDBHandler::putKepler(KeplerData kep)
    {
        replaceAllStr(kep.name, "'", "''");
        std::string sql = "INSERT INTO kepler (satellite_number," //
                          " element_number,"                      //
                          " name,"                                //
                          " designator,"                          //
                          " epoch,"                               //
                          " inclination,"                         //
                          " right_ascension,"                     //
                          " eccentricity,"                        //
                          " argument_of_perigee,"                 //
                          " mean_anomaly,"                        //
                          " mean_motion,"                         //
                          " derivative_mean_motion,"              //
                          " second_derivative_mean_motion,"       //
                          " bstar_drag_term,"                     //
                          " revolutions_at_epoch"                 //
                          ") VALUES ('" +
                          std::to_string(kep.satellite_number) + "', '" +              //
                          std::to_string(kep.element_number) + "', '" +                //
                          kep.name + "', '" +                                          //
                          kep.designator + "', '" +                                    //
                          std::to_string(kep.epoch) + "', '" +                         //
                          std::to_string(kep.inclination) + "', '" +                   //
                          std::to_string(kep.right_ascension) + "', '" +               //
                          std::to_string(kep.eccentricity) + "', '" +                  //
                          std::to_string(kep.argument_of_perigee) + "', '" +           //
                          std::to_string(kep.mean_anomaly) + "', '" +                  //
                          std::to_string(kep.mean_motion) + "', '" +                   //
                          std::to_string(kep.derivative_mean_motion) + "', '" +        //
                          std::to_string(kep.second_derivative_mean_motion) + "', '" + //
                          std::to_string(kep.bstar_drag_term) + "', '" +               //
                          std::to_string(kep.revolutions_at_epoch) + "'" +             //
                          "');";

        char *err = NULL;
        if (sqlite3_exec(h->db, sql.c_str(), NULL, 0, &err))
        {
            logger->error("Error inserting TLE in database! %s (%s)", err, sql.c_str());
            sqlite3_free(err);
        }
    }

#if 0
    std::optional<TLE> KeplerDBHandler::get_from_norad(int norad)
    {
        TLE r;
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(h->db, ("SELECT name,line1,line2,time from tle where norad=" + std::to_string(norad)).c_str(), -1, &res, 0))
            logger->error("Couldn't fetch TLE from DB! " + std::string(sqlite3_errmsg(h->db)));
        else if (sqlite3_step(res) == SQLITE_ROW)
        {
            r.norad = norad;
            r.name = (char *)sqlite3_column_text(res, 0);
            r.line1 = (char *)sqlite3_column_text(res, 1);
            r.line2 = (char *)sqlite3_column_text(res, 2);
            r.time = sqlite3_column_int64(res, 3);
        }

        sqlite3_finalize(res);
        return r;
    }

    std::optional<TLE> KeplerDBHandler::get_from_norad_time(int norad, time_t timestamp)
    {
        auto t = get_from_spacetrack_time(norad, timestamp);
        if (t.has_value())
            return t;
        else
            return get_from_norad(norad);
    }

    std::vector<TLE> KeplerDBHandler::get_all_tles()
    {
        std::vector<TLE> rr;
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(h->db, "select norad,name,line1,line2,time from tle;", -1, &res, 0))
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
                r.time = sqlite3_column_int64(res, 4);
                rr.push_back(r);
            }
        }

        sqlite3_finalize(res);
        return rr;
    }
#endif
} // namespace satdump