#include "mwr_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "logger.h"

#include <array>
#include <cstdint>
#include <cstdio>

namespace aws
{
    namespace mwr
    {
        MWRReader::MWRReader()
        {
            lines = 0;
            timestamps.resize(2, -1);
        }

        MWRReader::~MWRReader()
        {
            for (int i = 0; i < 19; i++)
                channels[i].clear();
        }

        double parseCUC(uint8_t *dat)
        {
            double seconds = dat[1] << 24 | dat[2] << 16 | dat[3] << 8 | dat[4];
            double fseconds = dat[5] << 16 | dat[6] << 8 | dat[7];
            double timestamp = seconds + fseconds / 16777215.0 + 3657 * 24 * 3600;
            if (timestamp > 0)
                return timestamp;
            else
                return -1;
        }

        // Extract calibration telemetry
        void MWRReader::parseCal()
        { // TODOREWORK cleanup, and take into account reflector temperature!!!!!!!!
            uint8_t *calib = &wip_full_pkt[70];

            uint16_t icu_voltage = calib[17] << 8 | calib[16];
            uint16_t icu_current = calib[19] << 8 | calib[18];

            double load1 = (calib[45] << 8 | calib[44]) / 3e3;
            double load2 = (calib[47] << 8 | calib[46]) / 3e3;
            double load3 = (calib[49] << 8 | calib[48]) / 3e3;
            double load4 = (calib[51] << 8 | calib[50]) / 3e3;

            int obct_n = wip_full_pkt[14 + 21] << 8 | wip_full_pkt[14 + 20];
            int cold_n = wip_full_pkt[14 + 25] << 8 | wip_full_pkt[14 + 24];
            int scene_n = wip_full_pkt[14 + 29] << 8 | wip_full_pkt[14 + 28];
            int acc_n = wip_full_pkt[14 + 31]; //<< 8 | wip_full_pkt[14 + 28];

            // logger->critical("12v Rail %d V %d A, T1 %f T2 %f T3 %f T4 %f  NUMOBCT %d NUMCOLD %d NUMSV %d ACC %d", icu_voltage, icu_current, load1, load2, load3, load4, obct_n, cold_n, scene_n,
            //                  acc_n);

            std::array<float, 4> v;
            v[0] = load1;
            v[1] = load2;
            v[2] = load3;
            v[3] = load4;
            cal_load_temps.push_back(v);

            std::array<std::array<uint16_t, 15>, 19> v2;
            for (int c = 0; c < 15; c++)
                for (int i = 0; i < 19; i++)
                    v2[i][c] = (wip_full_pkt[199 + (c * 40) + (i * 2) + 1] << 8) | wip_full_pkt[199 + (c * 40) + (i * 2) + 0];
            cal_load_views.push_back(v2);

            std::array<std::array<uint16_t, 25>, 19> v3;
            for (int c = 0; c < 25; c++)
                for (int i = 0; i < 19; i++)
                    v3[i][c] = (wip_full_pkt[799 + (c * 40) + (i * 2) + 1] << 8) | wip_full_pkt[799 + (c * 40) + (i * 2) + 0];
            cal_cold_views.push_back(v3);
        }

        void MWRReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 19)
                return;

            if (pkt.header.sequence_flag == 0b01)
            {
                if (wip_full_pkt.size() > 0)
                {
                    wip_full_pkt.resize(7599);

                    for (int c = 0; c < 145; c++)
                        for (int i = 0; i < 19; i++)
                            channels[i].push_back((wip_full_pkt[1799 + (c * 40) + (i * 2) + 1] << 8) | wip_full_pkt[1799 + (c * 40) + (i * 2) + 0]);
                    lines++;

                    double timestamp = parseCUC(wip_full_pkt.data() + 191);
                    if (ccsds::crcCheckCCITT(pkt))
                        timestamps.push_back(timestamp);
                    else
                        timestamps.push_back(-1);

                    parseCal();
                }
                wip_full_pkt.clear();

                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);
            }
            else if (pkt.header.sequence_flag == 0b00)
            {
                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);
            }
            else if (pkt.header.sequence_flag == 0b10)
            {
                wip_full_pkt.insert(wip_full_pkt.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);
                if (wip_full_pkt.size() > 0)
                {
                    wip_full_pkt.resize(7599);

                    for (int c = 0; c < 145; c++)
                        for (int i = 0; i < 19; i++)
                            channels[i].push_back((wip_full_pkt[1799 + (c * 40) + (i * 2) + 1] << 8) | wip_full_pkt[1799 + (c * 40) + (i * 2) + 0]);
                    lines++;

                    double timestamp = parseCUC(wip_full_pkt.data() + 191);
                    if (ccsds::crcCheckCCITT(pkt))
                        timestamps.push_back(timestamp);
                    else
                        timestamps.push_back(-1);

                    parseCal();
                }
                wip_full_pkt.clear();
            }
        }

        image::Image MWRReader::getChannel(int channel)
        {
            auto img = image::Image(channels[channel].data(), 16, 145, lines, 1);
            img.mirror(true, false);
            return img;
        }
    } // namespace mwr
} // namespace aws