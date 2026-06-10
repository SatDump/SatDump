#include "kepler_handler.h"
#include "core/config.h"
#include "db/db_handler.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "utils/format.h"
#include "utils/string.h"
#include <string>

namespace satdump
{
    KeplerDBHandler::KeplerDBHandler(std::shared_ptr<DBHandler> h) : DBHandlerBase(h) {}

    void KeplerDBHandler::init()
    {
        // Create Kepler Table
        std::string sql_create_kep = "CREATE TABLE IF NOT EXISTS kepler("
                                     "id TEXT PRIMARY KEY NOT NULL,"
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
        autoUpdateKeplers();
    }

    void KeplerDBHandler::autoUpdateKeplers()
    {
        std::string update_setting = "Never";
        satdump_cfg.tryAssignValueFromSatDumpGeneral(update_setting, "kepler_update_interval");
        time_t last_update = std::stoull(h->get_meta("kepler_last_updated", "0"));
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
            logger->error("Invalid Kepler Auto-update interval: %s", update_setting.c_str());
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
            eventBus->register_handler<AutoUpdateKeplersEvent>([this](AutoUpdateKeplersEvent evt) { updateKeplerDatabase(); });
            std::shared_ptr<AutoUpdateKeplersEvent> evt = std::make_shared<AutoUpdateKeplersEvent>();
            taskScheduler->add_task<AutoUpdateKeplersEvent>("auto_kepler_update_todorework", evt, last_update, update_interval);
        }
    }

    void KeplerDBHandler::updateKeplerDatabase()
    {
        logger->info("Updating Keplers...");
        std::vector<int> norads_to_fetch = satdump_cfg.main_cfg["kepler_settings"]["norads_to_fetch"].get<std::vector<int>>();
        std::vector<std::string> urls_to_fetch = satdump_cfg.main_cfg["kepler_settings"]["urls_to_fetch"].get<std::vector<std::string>>();

        for (auto &url_str : urls_to_fetch)
        {
            auto keps = tryFetchOMMFileFromURL(url_str);

            h->tr_begin();
            for (auto &t : keps)
                putKepler(t);
            h->tr_end();
        }

        for (int norad : norads_to_fetch)
        {
            auto tles = tryFetchSingleOMMwithNorad(norad);
            if (tles.size() == 1)
                putKepler(tles[0]);
            else
                logger->error("There should only be one Kepler per norad! %d (%d)", norad, tles.size());
        }

        {
            time_t tt = time(0);
            std::vector<int> norads;
            for (auto &t : get_all_tles())
                if (tt - t.time > (3600 * 24 * 2)) // TODOREWORK respect update interval!
                    norads.push_back(t.norad);

            if (norads.size())
                logger->error("%d Keplers are too old in database, even after attempting an update. Pulling from space-track. This is NOT optimal!", norads.size());

            while (norads.size() > 0)
            {
                std::vector<int> cnorads = norads;
                cnorads.resize(std::min<int>(2000, cnorads.size()));
                norads.erase(norads.begin(), norads.begin() + cnorads.size());
                auto tles = get_from_spacetrack_latest_list(cnorads); // tryFetchTLEsFromFileURL(url_str);

                logger->info("Got %d keplers from space-track!", tles.size());

                h->tr_begin();
                for (auto &t : tles)
                {
                    KeplerData kep;
                    if (tleToKepler(t, kep))
                        putKepler(kep);
                }
                h->tr_end();
            }
        }

        // Update last update timestamp & other stuff
        h->set_meta("kepler_last_updated", std::to_string(time(0)));
        logger->info("%d Keplers in database!", h->get_table_size("kepler"));
        all_.clear();
        eventBus->fire_event<TLEsUpdatedEvent>(TLEsUpdatedEvent());
    }

    void KeplerDBHandler::putKepler(KeplerData kep)
    {
        replaceAllStr(kep.name, "'", "''");
        std::string sql = "INSERT INTO kepler (id,"         //
                          " satellite_number,"              //
                          " element_number,"                //
                          " name,"                          //
                          " designator,"                    //
                          " epoch,"                         //
                          " inclination,"                   //
                          " right_ascension,"               //
                          " eccentricity,"                  //
                          " argument_of_perigee,"           //
                          " mean_anomaly,"                  //
                          " mean_motion,"                   //
                          " derivative_mean_motion,"        //
                          " second_derivative_mean_motion," //
                          " bstar_drag_term,"               //
                          " revolutions_at_epoch"           //
                          ") VALUES ('" +
                          to_string_with_precision(kep.satellite_number, 30) + "_" + to_string_with_precision(kep.epoch, 30) + "', '" + //
                          to_string_with_precision(kep.satellite_number, 30) + "', '" +                                                 //
                          to_string_with_precision(kep.element_number, 30) + "', '" +                                                   //
                          kep.name + "', '" +                                                                                           //
                          kep.designator + "', '" +                                                                                     //
                          to_string_with_precision(kep.epoch, 30) + "', '" +                                                            //
                          to_string_with_precision(kep.inclination, 30) + "', '" +                                                      //
                          to_string_with_precision(kep.right_ascension, 30) + "', '" +                                                  //
                          to_string_with_precision(kep.eccentricity, 30) + "', '" +                                                     //
                          to_string_with_precision(kep.argument_of_perigee, 30) + "', '" +                                              //
                          to_string_with_precision(kep.mean_anomaly, 30) + "', '" +                                                     //
                          to_string_with_precision(kep.mean_motion, 30) + "', '" +                                                      //
                          to_string_with_precision(kep.derivative_mean_motion, 30) + "', '" +                                           //
                          to_string_with_precision(kep.second_derivative_mean_motion, 30) + "', '" +                                    //
                          to_string_with_precision(kep.bstar_drag_term, 30) + "', '" +                                                  //
                          to_string_with_precision(kep.revolutions_at_epoch, 30) + "'" +                                                //
                          ") ON CONFLICT(id) DO UPDATE SET " +                                                                          //
                          "element_number='" + to_string_with_precision(kep.element_number, 30) + "', " +                               //
                          "name='" + kep.name + "', " +                                                                                 //
                          "designator='" + kep.designator + "', " +                                                                     //
                          "epoch='" + to_string_with_precision(kep.epoch, 30) + "', " +                                                 //
                          "inclination='" + to_string_with_precision(kep.inclination, 30) + "', " +                                     //
                          "right_ascension='" + to_string_with_precision(kep.right_ascension, 30) + "', " +                             //
                          "eccentricity='" + to_string_with_precision(kep.eccentricity, 30) + "', " +                                   //
                          "argument_of_perigee='" + to_string_with_precision(kep.argument_of_perigee, 30) + "', " +                     //
                          "mean_anomaly='" + to_string_with_precision(kep.mean_anomaly, 30) + "', " +                                   //
                          "mean_motion='" + to_string_with_precision(kep.mean_motion, 30) + "', " +                                     //
                          "derivative_mean_motion='" + to_string_with_precision(kep.derivative_mean_motion, 30) + "', " +               //
                          "second_derivative_mean_motion='" + to_string_with_precision(kep.second_derivative_mean_motion, 30) + "', " + //
                          "bstar_drag_term='" + to_string_with_precision(kep.bstar_drag_term, 30) + "', " +                             //
                          "revolutions_at_epoch='" + to_string_with_precision(kep.revolutions_at_epoch, 30) + "'" +                     //
                          ";";

        char *err = NULL;
        if (sqlite3_exec(h->db, sql.c_str(), NULL, 0, &err))
        {
            logger->error("Error inserting Kepler in database! %s (%s)", err, sql.c_str());
            sqlite3_free(err);
        }
    }

    bool KeplerDBHandler::getKepler(KeplerData &kep, int norad, time_t time)
    {
        bool ret = false;

        if (time == -1)
        {
            sqlite3_stmt *res;

            if (sqlite3_prepare_v2(h->db,
                                   ("select satellite_number, element_number, name, designator, epoch, inclination, right_ascension, eccentricity, argument_of_perigee, mean_anomaly, mean_motion, "
                                    "derivative_mean_motion, second_derivative_mean_motion, bstar_drag_term, revolutions_at_epoch from kepler where satellite_number=" +
                                    std::to_string(norad) + " order by epoch asc limit 1")
                                       .c_str(),
                                   -1, &res, 0))
                logger->error("Couldn't fetch Kepler data from DB! " + std::string(sqlite3_errmsg(h->db)));
            else if (sqlite3_step(res) == SQLITE_ROW)
            {
                kep.satellite_number = sqlite3_column_int(res, 0);
                kep.element_number = sqlite3_column_int(res, 1);
                kep.name = (char *)sqlite3_column_text(res, 2);
                kep.designator = (char *)sqlite3_column_text(res, 3);
                kep.epoch = sqlite3_column_double(res, 4);
                kep.inclination = sqlite3_column_double(res, 5);
                kep.right_ascension = sqlite3_column_double(res, 6);
                kep.eccentricity = sqlite3_column_double(res, 7);
                kep.argument_of_perigee = sqlite3_column_double(res, 8);
                kep.mean_anomaly = sqlite3_column_double(res, 9);
                kep.mean_motion = sqlite3_column_double(res, 10);
                kep.derivative_mean_motion = sqlite3_column_double(res, 11);
                kep.second_derivative_mean_motion = sqlite3_column_double(res, 12);
                kep.bstar_drag_term = sqlite3_column_double(res, 13);
                kep.revolutions_at_epoch = sqlite3_column_int(res, 14);

                ret = true;
            }

            sqlite3_finalize(res);
        }
        else
        {
            sqlite3_stmt *res;

            if (sqlite3_prepare_v2(h->db,
                                   ("select satellite_number, element_number, name, designator, epoch, inclination, right_ascension, eccentricity, argument_of_perigee, mean_anomaly, mean_motion, "
                                    "derivative_mean_motion, second_derivative_mean_motion, bstar_drag_term, revolutions_at_epoch from kepler where satellite_number=" +
                                    std::to_string(norad) + " order by abs(epoch - " + std::to_string(time) + ") desc limit 1")
                                       .c_str(),
                                   -1, &res, 0))
                logger->error("Couldn't fetch Kepler data from DB! " + std::string(sqlite3_errmsg(h->db)));
            else if (sqlite3_step(res) == SQLITE_ROW)
            {
                kep.satellite_number = sqlite3_column_int(res, 0);
                kep.element_number = sqlite3_column_int(res, 1);
                kep.name = (char *)sqlite3_column_text(res, 2);
                kep.designator = (char *)sqlite3_column_text(res, 3);
                kep.epoch = sqlite3_column_double(res, 4);
                kep.inclination = sqlite3_column_double(res, 5);
                kep.right_ascension = sqlite3_column_double(res, 6);
                kep.eccentricity = sqlite3_column_double(res, 7);
                kep.argument_of_perigee = sqlite3_column_double(res, 8);
                kep.mean_anomaly = sqlite3_column_double(res, 9);
                kep.mean_motion = sqlite3_column_double(res, 10);
                kep.derivative_mean_motion = sqlite3_column_double(res, 11);
                kep.second_derivative_mean_motion = sqlite3_column_double(res, 12);
                kep.bstar_drag_term = sqlite3_column_double(res, 13);
                kep.revolutions_at_epoch = sqlite3_column_int(res, 14);

                ret = true;
            }

            sqlite3_finalize(res);
        }

        return ret;
    }

    std::vector<KeplerData> KeplerDBHandler::getAllNewestKepler()
    {
        std::vector<KeplerData> all_keps;

        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(
                h->db,
                "SELECT c1.satellite_number, c1.element_number, c1.name, c1.designator, c1.epoch, c1.inclination, c1.right_ascension, c1.eccentricity, c1.argument_of_perigee, c1.mean_anomaly, "
                "c1.mean_motion, c1.derivative_mean_motion, c1.second_derivative_mean_motion, c1.bstar_drag_term, c1.revolutions_at_epoch FROM kepler c1 JOIN (SELECT satellite_number, MAX(epoch) "
                "AS Maxepoch FROM kepler GROUP BY satellite_number) c2 ON c1.satellite_number = c2.satellite_number AND c1.epoch = c2.Maxepoch;",
                -1, &res, 0))
            logger->error("Couldn't fetch Kepler data from DB! " + std::string(sqlite3_errmsg(h->db)));
        else
        {
            while (sqlite3_step(res) == SQLITE_ROW)
            {
                KeplerData kep;

                kep.satellite_number = sqlite3_column_int(res, 0);
                kep.element_number = sqlite3_column_int(res, 1);
                kep.name = (char *)sqlite3_column_text(res, 2);
                kep.designator = (char *)sqlite3_column_text(res, 3);
                kep.epoch = sqlite3_column_double(res, 4);
                kep.inclination = sqlite3_column_double(res, 5);
                kep.right_ascension = sqlite3_column_double(res, 6);
                kep.eccentricity = sqlite3_column_double(res, 7);
                kep.argument_of_perigee = sqlite3_column_double(res, 8);
                kep.mean_anomaly = sqlite3_column_double(res, 9);
                kep.mean_motion = sqlite3_column_double(res, 10);
                kep.derivative_mean_motion = sqlite3_column_double(res, 11);
                kep.second_derivative_mean_motion = sqlite3_column_double(res, 12);
                kep.bstar_drag_term = sqlite3_column_double(res, 13);
                kep.revolutions_at_epoch = sqlite3_column_int(res, 14);

                all_keps.push_back(kep);
            }
        }

        sqlite3_finalize(res);

        return all_keps;
    }
} // namespace satdump