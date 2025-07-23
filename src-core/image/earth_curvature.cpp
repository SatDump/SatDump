#include "earth_curvature.h"
#include "logger.h"
#include "meta.h"
#include "products/image/channel_transform.h"
#include <cmath>

namespace satdump
{
    namespace image
    {
        namespace earth_curvature
        {
            const float EARTH_RADIUS = 6371.0f;

            /*
            This was mostly based off the following document :
            https://web.archive.org/web/20200110090856if_/http://ceeserver.cee.cornell.edu:80/wdp2/cee6150/Monograph/615_04_GeomCorrect_rev01.pdf
            */
            Image correct_earth_curvature(Image &image, float satellite_height, float swath, float resolution_km, std::vector<float> *foward_table, std::vector<float> *reverse_table)
            {
                float satellite_orbit_radius = EARTH_RADIUS + satellite_height; // Compute the satellite's orbit radius
                int corrected_width = round(swath / resolution_km);             // Compute the output image size, or number of samples from the imager
                float satellite_view_angle = swath / EARTH_RADIUS;              // Compute the satellite's view angle
                float edge_angle =
                    -atanf(EARTH_RADIUS * sinf(satellite_view_angle / 2) / ((cosf(satellite_view_angle / 2)) * EARTH_RADIUS - satellite_orbit_radius)); // Max angle relative to the satellite

                float *correction_factors = new float[corrected_width]; // Create a LUT to avoid recomputing on each row

                // Generate them
                for (int i = 0; i < corrected_width; i++)
                {
                    float angle = ((float(i) / float(corrected_width)) - 0.5f) * satellite_view_angle;                                  // Get the satellite's angle
                    float satellite_angle = -atanf(EARTH_RADIUS * sinf(angle) / ((cosf(angle))*EARTH_RADIUS - satellite_orbit_radius)); // Convert to an angle relative to earth
                    correction_factors[i] = image.width() * ((satellite_angle / edge_angle + 1.0f) / 2.0f);                             // Convert that to a pixel from the original image
                }

                Image output_image(image.depth(), corrected_width, image.height(), image.channels()); // Allocate output image

                if (foward_table != nullptr)
                    foward_table->resize(image.width(), -1);

                if (reverse_table != nullptr)
                    reverse_table->resize(corrected_width, -1);

                for (int channel = 0; channel < image.channels(); channel++)
                {
                    // Process each row
#pragma omp parallel for
                    for (int row = 0; row < (int)image.height(); row++)
                    {
                        for (int i = 0; i < corrected_width; i++)
                        {
#if 1
                            // printf("%d %f %f %d %d\n", i, correction_factors[i], fmod(correction_factors[i], 1), (int)correction_factors[i], (int)correction_factors[i] + 1);
                            int currPixel = correction_factors[i];
                            int nextPixel = correction_factors[i] + 1;
                            float fractionalPx = fmod(correction_factors[i], 1);

                            if ((size_t)nextPixel >= image.width())
                                nextPixel = image.width() - 1;

                            int px1 = image.get(channel, currPixel, row);
                            int px2 = image.get(channel, nextPixel, row);

                            int px = px1 * (1.0 - fractionalPx) + px2 * fractionalPx;

                            output_image.set(channel, i, row, px);

                            if (foward_table != nullptr)
                                (*foward_table)[currPixel] = i;
                            if (reverse_table != nullptr)
                                (*reverse_table)[i] = currPixel;
#else
                            int pixel_to_use = correction_factors[i]; // Input pixel to use, will get rounder automatically
                            output_image[channel_offset_output + row * corrected_width + i] = image[channel_offset + row * image.width() + pixel_to_use]; // Copy over that pixel!
                            if (foward_table != nullptr)
                                foward_table[pixel_to_use] = i;
#endif
                        }
                    }
                }

                if (foward_table != nullptr)
                {
                    float last_val = 0;
                    for (int i = 0; i < (int)image.width(); i++)
                    {
                        if ((*foward_table)[i] == -1)
                            (*foward_table)[i] = last_val;
                        last_val = (*foward_table)[i];
                    }
                }

                // Maybe we could do some more smoothing after the fact?
                // Will see later.

                delete[] correction_factors;

                return output_image;
            }

            image::Image perform_geometric_correction(image::Image img, bool &success, std::vector<float> *foward_table, std::vector<float> *reverse_table)
            {
                if (img.width() == 0)
                    return img;

                success = false;
                if (!has_metadata_proj_cfg(img))
                    return img;
                auto proj_cfg = get_metadata_proj_cfg(img);
                if (!proj_cfg.contains("corr_swath"))
                    return img;
                if (!proj_cfg.contains("corr_resol"))
                    return img;
                if (!proj_cfg.contains("corr_altit"))
                    return img;

                float swath = proj_cfg["corr_swath"].get<float>();
                float resol = proj_cfg["corr_resol"].get<float>();
                float altit = proj_cfg["corr_altit"].get<float>();
                success = true;

                if (proj_cfg.contains("corr_width"))
                {
                    if ((int)img.width() != proj_cfg["corr_width"].get<int>())
                    {
                        logger->debug("Image width mistmatch %d %d => RES %f/%f", proj_cfg["corr_width"].get<int>(), img.width(), resol, swath);
                        resol *= proj_cfg["corr_width"].get<int>() / float(img.width());
                    }
                }

                // We need to update metadata!
                bool table_was_null = reverse_table == nullptr;
                if (table_was_null)
                    reverse_table = new std::vector<float>();

                // Actual correction
                auto img2 = image::earth_curvature::correct_earth_curvature(img, altit, swath, resol, foward_table, reverse_table);

                // Update metadata, add transform
                std::vector<std::pair<double, double>> ix;
                for (size_t i = 0; i < reverse_table->size(); i++)
                    ix.push_back({i, (*reverse_table)[i]});
                double x1 = 0, y1 = 0;
                if (proj_cfg.contains("transform2"))
                {
                    x1 = proj_cfg["transform2"]["bx"].get<double>();
                    y1 = proj_cfg["transform2"]["by"].get<double>();
                }
                proj_cfg["transform2"] = ChannelTransform().init_affine_interpx(1, 1, x1, y1, ix);
                image::set_metadata_proj_cfg(img2, proj_cfg);

                // Cleanup
                if (table_was_null)
                    delete reverse_table;

                return img2;
            }
        } // namespace earth_curvature
    } // namespace image
} // namespace satdump