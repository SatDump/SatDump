#pragma once

#include "common/tracking/tle.h"
#include "core/exception.h"
#include "db/db_handler.h"

namespace satdump
{
    class TleDBHandler : public DBHandlerBase
    {
    private:
        struct AutoUpdateTLEsEvent
        {
        };

        void autoUpdateTLE();

        void putTLE(TLE tle);

    public:
        TleDBHandler(std::shared_ptr<DBHandler> h);

        void init();

        void updateTLEDatabase();

        std::optional<TLE> get_from_norad(int norad);
        std::optional<TLE> get_from_norad_time(int norad, time_t timestamp);

    private:
        std::vector<TLE> get_all_tles();

    public:
        std::vector<TLE> all; // TODOREWORK remove! Temporary!
    };
} // namespace satdump