#include "iers_handler.h"
#include "core/plugin.h"
#include "db/db_handler.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "utils/http.h"

namespace satdump
{
    IersDBHandler::IersDBHandler(std::shared_ptr<DBHandler> h) : DBHandlerBase(h) {}

    void IersDBHandler::init()
    {
        // Create IERS Table
        std::string sql_create_iers = "CREATE TABLE IF NOT EXISTS iers("
                                      "time INT PRIMARY KEY NOT NULL,"
                                      "a_pole_x FLOAT NOT NULL,"
                                      "a_pole_y FLOAT NOT NULL,"
                                      "ut1_utc FLOAT NOT NULL);";
        if (h->run_sql(sql_create_iers))
            throw satdump_exception("Failed creating IERS database!");

        time_t last_update = std::stoull(h->get_meta("iers_last_updated", "0"));
        bool honor_setting = true;
        time_t update_interval = 3600 * 60; // TODOREWORK

        // Update now, if needed
        time_t now = time(NULL);
        if ((honor_setting && now > last_update + update_interval) || h->get_table_size("iers") <= 0)
        {
            updateIERS();
            last_update = now;
        }

        // Schedule updates while running
        if (honor_setting)
        {
            eventBus->register_handler<AutoUpdateIersEvent>([this](AutoUpdateIersEvent) { updateIERS(); });
            std::shared_ptr<AutoUpdateIersEvent> evt = std::make_shared<AutoUpdateIersEvent>();
            taskScheduler->add_task<AutoUpdateIersEvent>("auto_iers_update", evt, last_update, update_interval);
        }
    }

    void IersDBHandler::updateIERS()
    {
        std::string url_str = "https://datacenter.iers.org/products/eop/rapid/standard/json/finals2000A.all.json"; // TODOREWORK

        logger->info("Downloading IERS Bulletins from " + url_str + "...");
        std::string res;
        if (perform_http_request(url_str, res))
        {
            logger->error("Error fetching IERS Bulletins!");
            return;
        }

        h->tr_begin();

        nlohmann::json j = nlohmann::json::parse(res);
        for (auto &v : j["EOP"]["data"]["timeSeries"])
        {
            try
            {
                if (v["dataEOP"]["pole"][0]["source"] == "BulletinA")
                {
                    time_t time = ((std::stod(v["time"]["MJD"].get<std::string>()) - 40587) * 86400);
                    double a_pole_x = std::stod(v["dataEOP"]["pole"][0]["X"].get<std::string>());
                    double a_pole_y = std::stod(v["dataEOP"]["pole"][0]["Y"].get<std::string>());
                    double ut1_utc = std::stod(v["dataEOP"]["UT"][0]["UT1-UTC"].get<std::string>());

                    std::string sql = "INSERT INTO iers (time, a_pole_x, a_pole_y, ut1_utc) VALUES (" + std::to_string(time) + ", \"" + std::to_string(a_pole_x) + "\", '" + std::to_string(a_pole_y) +
                                      "', '" + std::to_string(ut1_utc) + "') ON CONFLICT(time) DO UPDATE SET a_pole_x=\"" + std::to_string(a_pole_x) + "\", a_pole_y='" + std::to_string(a_pole_y) +
                                      "', ut1_utc='" + std::to_string(ut1_utc) + "';";

                    char *err = NULL;
                    if (sqlite3_exec(h->db, sql.c_str(), NULL, 0, &err))
                    {
                        logger->error(err);
                        sqlite3_free(err);
                    }
                }
                else
                {
                    logger->error("First bulletin should be A!");
                }
            }
            catch (std::exception &e)
            {
                // logger->error("Error parsing IERS data! %s", e.what());
            }
        }

        h->tr_end();

        // Update last update timestamp & other stuff
        h->set_meta("iers_last_updated", std::to_string(time(0)));
        logger->info("%d IERS Records in database!", h->get_table_size("iers"));
    }

    IERSInfo IersDBHandler::getBestIERSInfo(double time)
    {
        IERSInfo r;
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(h->db, ("select a_pole_x,a_pole_y,ut1_utc from iers order by abs(" + std::to_string(time) + ") asc limit 1").c_str(), -1, &res, 0))
            logger->error("Couldn't fetch IERS data from DB! " + std::string(sqlite3_errmsg(h->db)));
        else if (sqlite3_step(res) == SQLITE_ROW)
        {
            r.polar_dx = sqlite3_column_double(res, 0) * 1000; // todorework to marcsec
            r.polar_dy = sqlite3_column_double(res, 1) * 1000; // todorework to marcsec
            r.ut1_utc = sqlite3_column_double(res, 2);
        }

        sqlite3_finalize(res);
        return r;
    }
} // namespace satdump