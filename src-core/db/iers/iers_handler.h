#pragma once

#include "common/tracking/tle.h"
#include "core/exception.h"
#include "db/db_handler.h"

namespace satdump
{
    struct IERSInfo
    {
        time_t time;
        double polar_dx;
        double polar_dy;
        double ut1_utc;
        int leap_seconds;
    };

    class IersDBHandler : public DBHandlerBase
    {
    private:
        struct AutoUpdateIersEvent
        {
        };

    public:
        IersDBHandler(std::shared_ptr<DBHandler> h);

        void init();
        void updateIERS();

        IERSInfo getBestIERSInfo(double time);
    };
} // namespace satdump