/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "common/tracking/tle.h"
#include "core/exception.h"
#include "init.h"
#include "logger.h"

#include <chrono>
#include <sqlite3.h>
#include <string>
#include <thread>
#include <unistd.h>

class SQLiteHandler
{
private:
    sqlite3 *db = nullptr;

    bool run_sql(std::string sql)
    {
        char *err = NULL;
        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
        {
            logger->error(err);
            sqlite3_free(err);
            return true;
        }
        return false;
    }

public:
    SQLiteHandler(std::string path)
    {
        // Open
        if (sqlite3_open(path.c_str(), &db))
            throw satdump_exception("Couldn't open SQLite database! " + std::string(sqlite3_errmsg(db)));
        else
            logger->info("Opened SQLite Database!");
        sqlite3_busy_timeout(db, 1000);

        // Init
        if (run_sql("PRAGMA journal_mode=WAL;"))
            throw satdump_exception("Failed setting database to WAL mode!");

        // Create TLE Table
        std::string sql_create_tle = "CREATE TABLE IF NOT EXISTS tle("
                                     "norad INT PRIMARY KEY NOT NULL,"
                                     "name TEXT NOT NULL,"
                                     "line1 TEXT NOT NULL,"
                                     "line2 TEXT);";
        if (run_sql(sql_create_tle))
            throw satdump_exception("Failed creating TLE database!");

        // Create general settings table
        std::string sql_create_settings = "CREATE TABLE IF NOT EXISTS settings("
                                          "id TEXT PRIMARY KEY NOT NULL,"
                                          "val TEXT NOT NULL);";
        if (run_sql(sql_create_settings))
            throw satdump_exception("Failed creating Settings database!");
    }

    ~SQLiteHandler() { sqlite3_close(db); }

    sqlite3 *getdb() { return db; }

    satdump::TLE get_tle_from_norad(int norad)
    {
        satdump::TLE r;
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(db, ("SELECT name,line1,line2 from tle where norad=" + std::to_string(norad)).c_str(), -1, &res, 0))
            throw satdump_exception("Couldn't fetch TLE from DB! " + std::string(sqlite3_errmsg(db)));

        if (sqlite3_step(res) == SQLITE_ROW)
        {
            r.norad = norad;
            r.name = (char *)sqlite3_column_text(res, 0);
            r.line1 = (char *)sqlite3_column_text(res, 1);
            r.line2 = (char *)sqlite3_column_text(res, 2);
        }

        sqlite3_finalize(res);
        return r;
    }
};

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    SQLiteHandler d("test.db");

    auto db = d.getdb();

    {
        nlohmann::json v;
        v["test"] = 4384328;
        v["v2"] = "djisoadjsoa";
        v["df"] = 5.66546;
        char *err = NULL;
        std::string sql = "INSERT INTO settings (id, val) VALUES ('testval', '" + v.dump() + "') ON CONFLICT(id) DO UPDATE SET val='" + v.dump() + "';";
        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
        {
            logger->error(err);
            sqlite3_free(err);
        }
    }

#if 1
    {
        {
            char *err = NULL;
            std::string sql = "BEGIN;";
            if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
            {
                logger->error(err);
                sqlite3_free(err);
            }
        }

        logger->info("INSERT");
        for (auto &tle : *satdump::general_tle_registry)
        {
            std::string sql = "INSERT INTO tle (norad, name, line1, line2) VALUES (" + std::to_string(tle.norad) + ", \"" + tle.name + "\", '" + tle.line1 + "', '" + tle.line2 +
                              "') ON CONFLICT(norad) DO UPDATE SET name=\"" + tle.name + "\", line1='" + tle.line1 + "', line2='" + tle.line2 + "';";

            char *err = NULL;
            if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
            {
                logger->error(err);
                logger->info(nlohmann::json(tle).dump());
                sqlite3_free(err);
            }
        }
        logger->info("DONE");

        {
            char *err = NULL;
            std::string sql = "END;";
            if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
            {
                logger->error(err);
                sqlite3_free(err);
            }
        }
    }
#endif

    auto tle = d.get_tle_from_norad(40069);
    logger->info(nlohmann::json(tle).dump());
}
