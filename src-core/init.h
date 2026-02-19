#pragma once

#include "db/iers/iers_handler.h"
#include "db/tle/tle_handler.h"
#include "dll_export.h"
#include <memory>
#include <string>

#include "db/db_handler.h"

namespace satdump
{
    SATDUMP_DLL extern std::string user_path;
    SATDUMP_DLL extern std::string tle_file_override;
    SATDUMP_DLL extern bool tle_do_update_on_init;
    void initSatDump(bool is_gui = false);
    void exitSatDump();

    SATDUMP_DLL extern std::shared_ptr<DBHandler> db;
    SATDUMP_DLL extern std::shared_ptr<TleDBHandler> db_tle;
    SATDUMP_DLL extern std::shared_ptr<IersDBHandler> db_iers;
} // namespace satdump