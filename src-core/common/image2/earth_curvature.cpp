#include "earth_curvature.h"
#include <cmath>

namespace image2
{
    namespace earth_curvature
    {
        const float EARTH_RADIUS = 6371.0f;

        /*
        This was mostly based off the following document :
        https://web.archive.org/web/20200110090856if_/http://ceeserver.cee.cornell.edu:80/wdp2/cee6150/Monograph/615_04_GeomCorrect_rev01.pdf
        */
        Image correct_earth_curvature(Image &image, float satellite_height, float swath, float resolution_km, float *foward_table)
        {
            float satellite_orbit_radius = EARTH_RADIUS + satellite_height;                                                                                        // Compute the satellite's orbit radius
            int corrected_width = round(swath / resolution_km);                                                                                                    // Compute the output image size, or number of samples from the imager
            float satellite_view_angle = swath / EARTH_RADIUS;                                                                                                     // Compute the satellite's view angle
            float edge_angle = -atanf(EARTH_RADIUS * sinf(satellite_view_angle / 2) / ((cosf(satellite_view_angle / 2)) * EARTH_RADIUS - satellite_orbit_radius)); // Max angle relative to the satellite

            float *correction_factors = new float[corrected_width]; // Create a LUT to avoid recomputing on each row

            // Generate them
            for (int i = 0; i < corrected_width; i++)
            {
                float angle = ((float(i) / float(corrected_width)) - 0.5f) * satellite_view_angle;                                    // Get the satellite's angle
                float satellite_angle = -atanf(EARTH_RADIUS * sinf(angle) / ((cosf(angle)) * EARTH_RADIUS - satellite_orbit_radius)); // Convert to an angle relative to earth
                correction_factors[i] = image.width() * ((satellite_angle / edge_angle + 1.0f) / 2.0f);                               // Convert that to a pixel from the original image
            }

            Image output_image(image.depth(), corrected_width, image.height(), image.channels()); // Allocate output image

            if (foward_table != nullptr)
                for (int i = 0; i < (int)image.width(); i++)
                    foward_table[i] = -1;

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
                            foward_table[currPixel] = i;
#else
                        int pixel_to_use = correction_factors[i];                                                                                     // Input pixel to use, will get rounder automatically
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
                    if (foward_table[i] == -1)
                        foward_table[i] = last_val;
                    last_val = foward_table[i];
                }
            }

            // Maybe we could do some more smoothing after the fact?
            // Will see later.

            delete[] correction_factors;

            return output_image;
        }
    }
}