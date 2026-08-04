#pragma once

/**
 * @file skl.h
 */

#include "ggak.h"
#include <cstddef>
#include <vector>

namespace elektro_arktika
{
    namespace ggak
    {
        enum skif_record_ch_t
        {
            // Main SKIF
            SKIF_1,
            SKIF_2,
            SKIF_3,

            // SKIF Spectral Electrons
            SKIF_SPECTRAL_E_1,
            SKIF_SPECTRAL_E_2,
            SKIF_SPECTRAL_E_3,
            SKIF_SPECTRAL_E_4,
            SKIF_SPECTRAL_E_5,
            SKIF_SPECTRAL_E_6,
            SKIF_SPECTRAL_E_7,
            SKIF_SPECTRAL_E_8,
            SKIF_SPECTRAL_E_9,
            SKIF_SPECTRAL_E_10,

            // SKIF Spectral Protons
            SKIF_SPECTRAL_P_1,
            SKIF_SPECTRAL_P_2,
            SKIF_SPECTRAL_P_3,
            SKIF_SPECTRAL_P_4,
            SKIF_SPECTRAL_P_5,
            SKIF_SPECTRAL_P_6,
            SKIF_SPECTRAL_P_7,
            SKIF_SPECTRAL_P_8,
            SKIF_SPECTRAL_P_9,
            SKIF_SPECTRAL_P_10,

            // Unknown
            SKIF_UKN,
        };

        skif_record_ch_t getSKIFChannel(int id, int pos, double *scale = NULL);

        struct SKIFRecord
        {
            double time;
            skif_record_ch_t type;
            double val;
        };

        /**
         * @brief Parse a SKIF frame from raw data.
         * All channels are "interleaved" in some weird way
         * with different sampling frequencies.
         *
         * @param frm GGAK Frame (ID 32)
         * @return parsed SKIF records
         */
        std::vector<SKIFRecord> parseSKIFRecord(GGAKFrame *frm);
    } // namespace ggak
} // namespace elektro_arktika