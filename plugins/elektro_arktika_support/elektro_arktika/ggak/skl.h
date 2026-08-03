#pragma once

/**
 * @file skl.h
 */

#include "ggak.h"
#include <vector>

namespace elektro_arktika
{
    namespace ggak
    {
        struct SKLRecord
        {
            double time;
            double dat[12];
        };

        /**
         * @brief Parse a SKL frm from raw data.
         * Channel 11 (12 in official) doesn't exist apparently!
         *
         * @param frm GGAK Frame (ID 48)
         * @return parsed SKL records (1s resolution)
         */
        std::vector<SKLRecord> parseSKLRecord(GGAKFrame *frm);

        std::vector<SKLRecord> resampleSKLTo10s(std::vector<SKLRecord> recs);
    } // namespace ggak
} // namespace elektro_arktika