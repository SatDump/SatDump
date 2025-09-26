#pragma once

#include <memory>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace satdump
{
    class DBHandler;

    class DBHandlerBase
    {
    protected:
        std::shared_ptr<DBHandler> h;

    public:
        DBHandlerBase(std::shared_ptr<DBHandler> h) : h(h) {}
        virtual void init() = 0;
    };

    class DBHandler
    {
    public:
        sqlite3 *db = nullptr;
        std::vector<std::shared_ptr<DBHandlerBase>> subhandlers;

        bool run_sql(std::string sql)
        {
            char *err = NULL;
            if (sqlite3_exec(db, sql.c_str(), NULL, 0, &err))
            {
                // TODOREWORK logger->error(err);
                sqlite3_free(err);
                return true;
            }
            return false;
        }

    public:
        void set_meta(std::string id, std::string val);
        std::string get_meta(std::string id, std::string def = "");

        int get_table_size(std::string table);

        void tr_begin();
        void tr_end();

    public:
        DBHandler(std::string path);
        void init();
        ~DBHandler();
    };
} // namespace satdump