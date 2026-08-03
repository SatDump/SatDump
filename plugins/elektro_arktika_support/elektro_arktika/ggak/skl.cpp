#include "skl.h"
#include "elektro_arktika/ggak/ggak.h"
#include <cmath>

namespace elektro_arktika
{
    namespace ggak
    {
        std::vector<SKLRecord> parseSKLRecord(GGAKFrame *frm)
        {
            std::vector<SKLRecord> recs;

            for (int i = 0; i < 14; i++)
            {
                uint8_t *ptr = frm->payload + i * 14;

                SKLRecord rec;

                rec.time = getTimestamp(frm);

                for (int c = 0; c < 11; c++)
                {
                    if (c == 10)
                        rec.dat[11] = double((ptr[3 + c])) * 10;
                    else if (c == 0 || c == 9)
                        rec.dat[c] = exp(double((ptr[3 + c]) - 67) / (29.02)) * 100 * 10;
                    else
                        rec.dat[c] = double((ptr[3 + c])) * 10;
                }

                rec.dat[10] = -9.99999e+99;

                recs.push_back(rec);
            }

            return recs;
        }

        std::vector<SKLRecord> resampleSKLTo10s(std::vector<SKLRecord> recs)
        {
            auto v = recs;

            recs.clear();
            SKLRecord wip;

            int cv = 0;

            for (auto &vv : v)
            {
                if (cv == 0)
                {
                    wip.time = vv.time;
                    for (int i = 0; i < 12; i++)
                        wip.dat[i] = 0;
                }

                for (int i = 0; i < 12; i++)
                    wip.dat[i] += vv.dat[i];

                cv++;

                if (cv == 10)
                {
                    for (int i = 0; i < 12; i++)
                        wip.dat[i] /= 10;
                    recs.push_back(wip);
                    cv = 0;
                }
            }

            return recs;
        }
    } // namespace ggak
} // namespace elektro_arktika