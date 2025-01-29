#pragma once

#include "products2/image/image_calibrator.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include "common/calibration.h"
#include "common/utils.h"

namespace meteor
{
    class MeteorMsuMrCalibrator : public satdump::products::ImageCalibrator
    {
    private:
        bool lrpt;
        // todo
        std::vector<double> wavenumbers;

        std::vector<std::vector<std::pair<uint16_t, uint16_t>>> views;
        std::vector<double> cold_temps;
        std::vector<double> hot_temps;

        bool can_vis_calib = false;
        bool can_ir_calib = false;
        double vis_coeffs[3][2];

    public:
        MeteorMsuMrCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
        {
            if (d_cfg["vars"].contains("vis"))
            {
                for (int c = 0; c < 3; c++)
                {
                    vis_coeffs[c][0] = d_cfg["vars"]["vis"][c][0];
                    vis_coeffs[c][1] = d_cfg["vars"]["vis"][c][1];
                }
                can_vis_calib = true;
            }

            lrpt = getValueOrDefault(d_cfg["vars"]["lrpt"], false);

            can_ir_calib = d_cfg["vars"].contains("temps") && d_cfg["vars"].contains("views");

            if (can_ir_calib)
            {
                views.resize(d_pro->images.size());

                int max_lcnt = 0;
                for (size_t i = 0; i < d_pro->images.size(); i++)
                {
                    wavenumbers.push_back(d_pro->get_channel_wavenumber(d_pro->images[i].abs_index));
                    int lcnt = d_cfg["vars"]["views"][i][0].size();
                    max_lcnt = std::max(lcnt, max_lcnt);
                    for (int j = 0; j < lcnt; j++)
                        views[i].push_back({d_cfg["vars"]["views"][i][0][j], d_cfg["vars"]["views"][i][1][j]});
                }

                for (int i = 0; i < max_lcnt; i++)
                {
                    double coldt = 0;
                    double hott = 0;
                    int next_x_calib = i;
                    while (next_x_calib < max_lcnt)
                    {
                        if (!d_cfg["vars"]["temps"][next_x_calib].is_null())
                        {
                            coldt = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp1"].get<double>() +
                                     d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp2"].get<double>()) /
                                    2.0;
                            hott = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp1"].get<double>() +
                                    d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp2"].get<double>()) /
                                   2.0;
                            break;
                        }
                        next_x_calib++;
                    }

                    if (coldt == 0 || hott == 0)
                    {
                        int next_x_calib = i;
                        while (0 < next_x_calib)
                        {
                            if (!d_cfg["vars"]["temps"][next_x_calib].is_null())
                            {
                                coldt = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp1"].get<double>() +
                                         d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["cold_temp2"].get<double>()) /
                                        2.0;
                                hott = (d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp1"].get<double>() +
                                        d_cfg["vars"]["temps"][next_x_calib]["analog_tlm"]["hot_temp2"].get<double>()) /
                                       2.0;
                                break;
                            }
                            next_x_calib--;
                        }
                    }

                    cold_temps.push_back(coldt);
                    hot_temps.push_back(hott);
                }

                auto coldm = most_common(cold_temps.begin(), cold_temps.end(), 0);
                auto hotm = most_common(hot_temps.begin(), hot_temps.end(), 0);
                for (int i = 0; i < max_lcnt; i++)
                {
                    if (abs(coldm - cold_temps[i]) > 5)
                        cold_temps[i] = coldm;
                    if (abs(hotm - hot_temps[i]) > 5)
                        hot_temps[i] = hotm;
                }
            }
        }

        double compute(int channel, int /*pos_x*/, int pos_y, uint32_t px_val)
        {
            if (wavenumbers[channel] == 0 || px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            if (lrpt)
            {
                pos_y /= 8; // Telemetry data covers 8 lines
                            // TODOREWORK                px_val = ((float)px_val / 255.0f) * 1023.0f; // Scale to 10-bit
            }

            if (channel < 3)
            {
                if (!can_vis_calib)
                    return CALIBRATION_INVALID_VALUE;

                double rad = 0;

                // Each time taken from point 573 / 47 (-1 since starts from 0)
                rad = ((px_val - 47.0) / (572.0 - 47.0)) * (vis_coeffs[channel][1] - vis_coeffs[channel][0]);

                if (rad < 0)
                    rad = 0;
                rad *= (299792458.0 / wavenumber_to_freq(wavenumbers[channel])) * 1e6;
                return rad;
            }
            else
            {
                if (!can_ir_calib)
                    return CALIBRATION_INVALID_VALUE;

                double cold_view = views[channel][pos_y].first;
                double hot_view = views[channel][pos_y].second;
                if (cold_view == 0 || hot_view == 0 || px_val == 0)
                    return CALIBRATION_INVALID_VALUE;

                double cold_ref = cold_temps[pos_y]; // 225;
                double hot_ref = hot_temps[pos_y];   // 312;
                double wavenumber = wavenumbers[channel];

                double cold_rad = temperature_to_radiance(cold_ref, wavenumber);
                double hot_rad = temperature_to_radiance(hot_ref, wavenumber);

                double gain_rad = (hot_rad - cold_rad) / (hot_view - cold_view);
                double rad = cold_rad + (px_val - cold_view) * gain_rad; // hot_rad + ((px_val - hot_view) / gain_rad);

                // printf("VIEW %f, %f %f %f %f (%f)=> %f\n", double(px_val), hot_view, cold_view, hot_rad, cold_rad, gain_rad, rad);

                return rad;
            }
        }
    };
}

/*

// FLOWCHART AND CODE USED TO FIND VIS CALIB COEFFS

#include "logger.h"
#include <fstream>
#include "common/image/io.h"

#include "common/image/processing.h"
#include "common/utils.h"

int main(int argc, char *argv[])
{
    initLogger();

    image::Image imgCAL, imgRAW;
    image::load_img(imgCAL, "/nvme_data/METEOR_CALIB/TOAL1_RAW.tif");
    image::load_img(imgRAW, "/nvme_data/METEOR_CALIB/MSU-MR/MSU-MR-3.png");

    // image::normalize(imgCAL);
    // image::save_img(imgCAL, "/nvme_data/METEOR_CALIB/BBP-74687949-20250128-3/MM24_MSUMR_20241221T132243_14216300/MM24_MSUMR_20241221T132243_14216300_N5139E01903_20241222_1D_TOAL1.png");

    std::vector<float> calLut[1024];

    for (size_t i = 0; i < imgCAL.width() * imgCAL.height(); i++)
    {
        uint16_t val = imgCAL.get(0, i);

        if (val != 0)
        {
            uint16_t rawVal = imgRAW.get(i) >> 6;
            float calV = val * 0.0028 - 0.0028;

            calLut[rawVal].push_back(calV);

            // printf("%d => %f \n", rawVal, calV);
        }
    }

    for (int i = 0; i < 1024; i++)
    {
        double rad = ((i - 47.0) / (572.0 - 47.0)) * (1.511627 - 0.003260);
        if (rad < 0)
            rad = 0;
        printf("%f,", rad); // avg_overflowless(calLut[i]));
        printf("%f,\n", avg_overflowless(calLut[i]));
    }
    printf("\n");
}


{
    "links": [
        {
            "end": 7,
            "id": 2,
            "start": 3
        },
        {
            "end": 9,
            "id": 4,
            "start": 6
        },
        {
            "end": 5,
            "id": 1,
            "start": 2
        },
        {
            "end": 1,
            "id": 0,
            "start": 8
        },
        {
            "end": 4,
            "id": 3,
            "start": 0
        }
    ],
    "nodes": [
        {
            "id": 0,
            "int_cfg": {
                "path": "/nvme_data/METEOR_CALIB/BBP-74687949-20250128-3/MM24_MSUMR_20241221T132243_14216300/MM24_MSUMR_20241221T132243_14216300_N5139E01903_20241222_1D_TOAL1.tif"
            },
            "int_id": "image_source",
            "io": [
                {
                    "id": 0,
                    "is_out": true,
                    "name": "Image"
                }
            ],
            "pos_x": 158.0,
            "pos_y": 278.0
        },
        {
            "id": 1,
            "int_cfg": null,
            "int_id": "image_get_proj",
            "io": [
                {
                    "id": 1,
                    "is_out": false,
                    "name": "Image"
                },
                {
                    "id": 2,
                    "is_out": true,
                    "name": "Projection"
                }
            ],
            "pos_x": 634.0,
            "pos_y": 510.0
        },
        {
            "id": 2,
            "int_cfg": {
                "path": "/nvme_data/METEOR_CALIB/MSU-MR/product.cbor"
            },
            "int_id": "image_product_source",
            "io": [
                {
                    "id": 3,
                    "is_out": true,
                    "name": "Product"
                }
            ],
            "pos_x": 30.0,
            "pos_y": 546.0
        },
        {
            "id": 3,
            "int_cfg": null,
            "int_id": "image_reproj",
            "io": [
                {
                    "id": 4,
                    "is_out": false,
                    "name": "Image"
                },
                {
                    "id": 5,
                    "is_out": false,
                    "name": "Projection"
                },
                {
                    "id": 6,
                    "is_out": true,
                    "name": "Image"
                }
            ],
            "pos_x": 849.0,
            "pos_y": 402.0
        },
        {
            "id": 4,
            "int_cfg": {
                "equation": "ch1"
            },
            "int_id": "image_product_equation",
            "io": [
                {
                    "id": 7,
                    "is_out": false,
                    "name": "Product"
                },
                {
                    "id": 8,
                    "is_out": true,
                    "name": "Image"
                }
            ],
            "pos_x": 321.0,
            "pos_y": 436.0
        },
        {
            "id": 5,
            "int_cfg": {
                "path": "/nvme_data/METEOR_CALIB/TOAL1_RAW.tif"
            },
            "int_id": "image_sink",
            "io": [
                {
                    "id": 9,
                    "is_out": false,
                    "name": "Image"
                }
            ],
            "pos_x": 1013.0,
            "pos_y": 421.0
        }
    ]
}

*/