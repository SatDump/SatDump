#pragma once

#include "db/iers/iers_handler.h"
#include "db/kepler/kepler_handler.h"
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

    struct StyleOrUINeedUpdateEvent
    {
    };

#if ENABLE_I18N
    void initLanguage(std::string lang = "");
    SATDUMP_DLL extern std::string current_language;
#endif

    SATDUMP_DLL extern std::shared_ptr<DBHandler> db;
    SATDUMP_DLL extern std::shared_ptr<KeplerDBHandler> db_keplers;
    SATDUMP_DLL extern std::shared_ptr<IersDBHandler> db_iers;
} // namespace satdump