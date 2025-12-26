#include "db_handler.h"
#include "core/exception.h"
#include "db/tle/tle_handler.h"
#include "logger.h"
#include "utils/string.h"

namespace satdump
{
    DBHandler::DBHandler(std::string path)
    {
        if (!std::filesystem::exists(std::filesystem::path(path).parent_path()) && std::filesystem::path(path).has_parent_path())
            std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        // Open
        if (sqlite3_open(path.c_str(), &db))
            throw satdump_exception("Couldn't open SQLite database! " + std::string(sqlite3_errmsg(db)));
        else
            logger->info("Opened SQLite Database!");
        sqlite3_busy_timeout(db, 1000);
    }

    void DBHandler::init()
    {
        // Create meta table used to store random simple fields
        std::string sql_create_meta = "CREATE TABLE IF NOT EXISTS meta("
                                      "id TEXT PRIMARY KEY NOT NULL,"
                                      "val TEXT NOT NULL);";
        if (run_sql(sql_create_meta))
            throw satdump_exception("Failed creating meta table!");

        // Create meta table used to store random simple fields
        std::string sql_create_user = "CREATE TABLE IF NOT EXISTS user("
                                      "id TEXT PRIMARY KEY NOT NULL,"
                                      "val TEXT NOT NULL);";
        if (run_sql(sql_create_user))
            throw satdump_exception("Failed creating user table!");

        // Let everything else do the same!
        logger->info("Setting up database...");
        for (auto &s : subhandlers)
            s->init();
    }

    DBHandler::~DBHandler()
    {
        if (sqlite3_close(db))
            logger->error("Error closing SQLite database!");
    }

    void DBHandler::set_meta(std::string id, std::string val)
    {
        char *err = NULL;
        replaceAllStr(val, "'", "''");
        std::string sql = "INSERT INTO meta (id, val) VALUES ('" + id + "', '" + val + "') ON CONFLICT(id) DO UPDATE SET val='" + val + "';";
        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
        {
            logger->error("Error setting value (%s = %s) in meta table! %s", id.c_str(), val.c_str(), err);
            sqlite3_free(err);
        }
    }

    std::string DBHandler::get_meta(std::string id, std::string def)
    {
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(db, ("SELECT val from meta where id=\"" + id + "\"").c_str(), -1, &res, 0))
            logger->warn(sqlite3_errmsg(db));
        else if (sqlite3_step(res) == SQLITE_ROW)
            def = (char *)sqlite3_column_text(res, 0);

        sqlite3_finalize(res);

        return def;
    }

    void DBHandler::set_user(std::string id, std::string val)
    {
        char *err = NULL;
        replaceAllStr(val, "'", "''");
        std::string sql = "INSERT INTO user (id, val) VALUES ('" + id + "', '" + val + "') ON CONFLICT(id) DO UPDATE SET val='" + val + "';";
        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
        {
            logger->error("Error setting value (%s = %s) in user table! %s", id.c_str(), val.c_str(), err);
            sqlite3_free(err);
        }
    }

    std::string DBHandler::get_user(std::string id, std::string def)
    {
        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(db, ("SELECT val from user where id=\"" + id + "\"").c_str(), -1, &res, 0))
            logger->warn(sqlite3_errmsg(db));
        else if (sqlite3_step(res) == SQLITE_ROW)
            def = (char *)sqlite3_column_text(res, 0);

        sqlite3_finalize(res);

        return def;
    }

    int DBHandler::get_table_size(std::string table)
    {
        sqlite3_stmt *res;
        int r = -1;

        if (sqlite3_prepare_v2(db, ("SELECT count(*) from " + table).c_str(), -1, &res, 0))
            logger->warn("Error getting table size! %s", sqlite3_errmsg(db));
        else if (sqlite3_step(res) == SQLITE_ROW)
            r = sqlite3_column_int(res, 0);

        sqlite3_finalize(res);

        return r;
    }

    void DBHandler::tr_begin()
    {
        char *err = NULL;
        std::string sql = "BEGIN;";
        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
        {
            logger->error("Error running BEGIN on DB %s", err);
            sqlite3_free(err);
        }
    }

    void DBHandler::tr_end()
    {
        char *err = NULL;
        std::string sql = "END;";
        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
        {
            logger->error("Error running END on DB %s", err);
            sqlite3_free(err);
        }
    }
} // namespace satdump