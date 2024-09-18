#pragma once

#include "core/exception.h"

namespace meteor
{
    enum dump_instrument_type_t
    {
        DUMP_TYPE_MTVZA,
        DUMP_TYPE_KMSS_BPSK,
    };

    inline dump_instrument_type_t parseDumpType(nlohmann::json params)
    {
        if (params["instrument_type"] == "mtvza")
            return DUMP_TYPE_MTVZA;
        else if (params["instrument_type"] == "kmss_bpsk")
            return DUMP_TYPE_KMSS_BPSK;
        else
            throw satdump_exception("Invalid METEOR instrument type!");
    }
}