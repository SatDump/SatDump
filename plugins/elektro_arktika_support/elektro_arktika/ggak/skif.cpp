#include "skif.h"
#include "elektro_arktika/ggak/ggak.h"
#include <cmath>
#include <cstddef>

namespace elektro_arktika
{
    namespace ggak
    {
        struct SKIFLutEntry
        {
            struct id_cfg_t
            {
                int first, second;
                double scale = 1;
            };

            skif_record_ch_t type;
            std::vector<id_cfg_t> ids;
        };

        std::vector<SKIFLutEntry> skif_channel_lut = //
            {
                {
                    SKIF_1,
                    {
                        {10, 3},  //
                        {133, 3}, //
                        {137, 3}, //
                        {2, 3},   //
                        {6, 3},   //
                        {130, 3}, //
                        {134, 3}, //
                        {138, 3}, //
                        {3, 3},   //
                        {7, 3},   //
                        {131, 3}, //
                        {135, 3}, //
                        {1, 3},   //
                        {4, 3},   //
                        {8, 3},   //
                        {132, 3}, //
                        {136, 3}, //
                        {193, 3}, //
                        {5, 3},   //
                        {9, 3},   //
                    },
                },
                {
                    SKIF_2,
                    {
                        {10, 4},  //
                        {133, 4}, //
                        {137, 4}, //
                        {2, 4},   //
                        {6, 4},   //
                        {130, 4}, //
                        {134, 4}, //
                        {138, 4}, //
                        {3, 4},   //
                        {7, 4},   //
                        {131, 4}, //
                        {135, 4}, //
                        {1, 4},   //
                        {4, 4},   //
                        {8, 4},   //
                        {132, 4}, //
                        {136, 4}, //
                        {193, 4}, //
                        {5, 4},   //
                        {9, 4},   //
                    },
                },
                {
                    SKIF_3,
                    {
                        {10, 5},       //
                        {133, 5},      //
                        {137, 5},      //
                        {2, 5},        //
                        {6, 5},        //
                        {130, 5},      //
                        {134, 5},      //
                        {138, 5},      //
                        {3, 5},        //
                        {7, 5},        //
                        {130, 9, 0.5}, //
                        {134, 9, 0.5}, //
                        {138, 9, 0.5}, //
                        {3, 9, 0.5},   //
                        {7, 9, 0.5},   //
                        {131, 5},      //
                        {135, 5},      //
                        {1, 5},        //
                        {4, 5},        //
                        {8, 5},        //
                        {132, 5},      //
                        {136, 5},      //
                        {193, 5},      //
                        {5, 5},        //
                        {9, 5},        //
                        {132, 9, 0.5}, //
                        {136, 9, 0.5}, //
                        {1, 9, 0.5},   //
                        {5, 9, 0.5},   //
                        {9, 9, 0.5},   //
                    },
                },

                ///////////////////

                {
                    SKIF_SPECTRAL_E_1,
                    {
                        {2, 10},   //
                        {4, 10},   //
                        {6, 10},   //
                        {8, 10},   //
                        {10, 10},  //
                        {131, 10}, //
                        {133, 10}, //
                        {135, 10}, //
                        {137, 10}, //
                        {193, 10}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_2,
                    {
                        {1, 1},   //
                        {3, 1},   //
                        {130, 1}, //
                        {131, 1}, //
                        {133, 1}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_3,
                    {
                        {2, 1},   //
                        {132, 1}, //
                        {193, 1}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_4,
                    {
                        {1, 6},   //
                        {3, 6},   //
                        {5, 6},   //
                        {7, 6},   //
                        {9, 6},   //
                        {130, 6}, //
                        {132, 6}, //
                        {134, 6}, //
                        {136, 6}, //
                        {138, 6}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_5,
                    {
                        {2, 6},   //
                        {4, 6},   //
                        {6, 6},   //
                        {8, 6},   //
                        {10, 6},  //
                        {131, 6}, //
                        {133, 6}, //
                        {135, 6}, //
                        {137, 6}, //
                        {193, 6}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_6,
                    {
                        {5, 1},   //
                        {135, 1}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_7,
                    {
                        {6, 1},   //
                        {136, 1}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_8,
                    {
                        {7, 1},   //
                        {137, 1}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_9,
                    {
                        {8, 1},   //
                        {138, 1}, //
                    },
                },
                {
                    SKIF_SPECTRAL_E_10,
                    {
                        {9, 1},  //
                        {10, 1}, //
                    },
                },

                ///////////////////

                {
                    SKIF_SPECTRAL_P_1,
                    {
                        {3, 10},   //
                        {5, 10},   //
                        {7, 10},   //
                        {9, 10},   //
                        {130, 10}, //
                        {132, 10}, //
                        {134, 10}, //
                        {136, 10}, //
                        {138, 10}, //
                    },
                },
                {
                    SKIF_SPECTRAL_P_2,
                    {
                        {4, 1},   //
                        {134, 1}, //
                    },
                },
                {
                    SKIF_SPECTRAL_P_3,
                    {
                        {2, 9},   //
                        {4, 9},   //
                        {6, 9},   //
                        {8, 9},   //
                        {10, 9},  //
                        {131, 9}, //
                        {133, 9}, //
                        {135, 9}, //
                        {137, 9}, //
                        {193, 9}, //
                    },
                },
                {
                    SKIF_SPECTRAL_P_4,
                    {
                        {1, 8},   //
                        {3, 8},   //
                        {5, 8},   //
                        {7, 8},   //
                        {9, 8},   //
                        {130, 8}, //
                        {132, 8}, //
                        {134, 8}, //
                        {136, 8}, //
                        {138, 8}, //
                    },
                },
                {
                    SKIF_SPECTRAL_P_5,
                    {
                        {10, 8},  //
                        {133, 8}, //
                        {137, 8}, //
                        {2, 8},   //
                        {6, 8},   //
                        {131, 8}, //
                        {135, 8}, //
                        {193, 8}, //
                        {4, 8},   //
                        {8, 8},   //
                    },
                },
                {
                    SKIF_SPECTRAL_P_6,
                    {
                        {10, 7, 0.5},  //
                        {133, 7, 0.5}, //
                        {137, 7, 0.5}, //
                        {2, 7, 0.5},   //
                        {6, 7, 0.5},   //
                        {130, 7},      //
                        {134, 7},      //
                        {138, 7},      //
                        {3, 7},        //
                        {7, 7},        //
                        {131, 7, 0.5}, //
                        {135, 7, 0.5}, //
                        {1, 7},        //
                        {4, 7, 0.5},   //
                        {8, 7, 0.5},   //
                        {132, 7},      //
                        {136, 7},      //
                        {193, 7, 0.5}, //
                        {5, 7},        //
                        {9, 7},        //

                    },
                },
                {
                    SKIF_SPECTRAL_P_7,
                    {
                        {1, 2},   //
                        {132, 2}, //
                        {135, 2}, //
                        {193, 2}, //
                        {4, 2},   //
                        {130, 2}, //
                        {133, 2}, //
                        {136, 2}, //
                        {2, 2},   //
                        {5, 2},   //
                        {131, 2}, //
                        {134, 2}, //
                        {137, 2}, //
                        {3, 2},   //
                        {6, 2},   //
                    },
                },
                {
                    SKIF_SPECTRAL_P_8,
                    {
                        {7, 2}, //
                    },
                },
                {
                    SKIF_SPECTRAL_P_9,
                    {
                        {8, 2},   //
                        {138, 2}, //
                    },
                },
                {
                    SKIF_SPECTRAL_P_10,
                    {
                        {9, 2},  //
                        {10, 2}, //
                    },
                },
        };

        skif_record_ch_t getSKIFChannel(int id, int pos, double *scale)
        {
            for (auto &ch : skif_channel_lut)
                for (auto &l : ch.ids)
                    if (l.first == id && l.second == pos)
                    {
                        if (scale != NULL)
                            *scale = l.scale;
                        return ch.type;
                    }
            return SKIF_UKN;
        }

        std::vector<SKIFRecord> parseSKIFRecord(GGAKFrame *frm)
        {
            std::vector<SKIFRecord> recs;

            for (int i = 0; i < 13; i++)
            {
                uint8_t *ptr = frm->payload + i * 15;

                int intid = ptr[3 + 0];

                for (int ii = 0; ii < 10; ii++)
                {
                    double scale = 1;
                    auto ch = getSKIFChannel(intid, ii + 1, &scale);

                    double val = 0;

                    if (ch == SKIF_1 || ch == SKIF_2 || ch == SKIF_3)
                        val = exp(double((ptr[3 + 2 + ii]) + 230.1144214) / 28.68) * scale;

                    else if (ch == SKIF_SPECTRAL_E_1)
                        val = ptr[3 + 2 + ii] * 17710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_2)
                        val = ptr[3 + 2 + ii] * 17710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_3)
                        val = ptr[3 + 2 + ii] * 12710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_4)
                        val = ptr[3 + 2 + ii] * 3710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_5)
                        val = ptr[3 + 2 + ii] * 3710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_6)
                        val = ptr[3 + 2 + ii] * 2710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_7)
                        val = ptr[3 + 2 + ii] * 1310; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_8)
                        val = ptr[3 + 2 + ii] * 910; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_9)
                        val = ptr[3 + 2 + ii] * 610; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_E_10)
                        val = ptr[3 + 2 + ii] * 220; // Prob also log, but idk

                    else if (ch == SKIF_SPECTRAL_P_1)
                        val = ptr[3 + 2 + ii] * 187710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_P_2)
                        val = ptr[3 + 2 + ii] * 187710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_P_3)
                        val = ptr[3 + 2 + ii] * 107710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_P_4)
                        val = ptr[3 + 2 + ii] * 107710; // Prob also log, but idk
                    else if (ch == SKIF_SPECTRAL_P_5)
                        val = exp(double((ptr[3 + 2 + ii]) + 180) / 14.5);
                    else if (ch == SKIF_SPECTRAL_P_6)
                        val = exp(double((ptr[3 + 2 + ii]) + 180) / 17.5) * scale;
                    else if (ch == SKIF_SPECTRAL_P_7)
                        val = exp(double((ptr[3 + 2 + ii]) + 200) / 22);
                    else if (ch == SKIF_SPECTRAL_P_8)
                        val = exp(double((ptr[3 + 2 + ii]) + 215) / 25);
                    else if (ch == SKIF_SPECTRAL_P_9)
                        val = exp(double((ptr[3 + 2 + ii]) + 200) / 25);
                    else if (ch == SKIF_SPECTRAL_P_10)
                        val = exp(double((ptr[3 + 2 + ii]) + 160) / 23);

                    else
                        val = exp(double((ptr[3 + 2 + ii]) + 230.1144214) / 28.68);

                    recs.push_back({getTimestamp(frm) + i, ch, val});
                }
            }

            return recs;
        }
    } // namespace ggak
} // namespace elektro_arktika