#include "earth_curvature.h"
#include <cmath>

namespace image
{
    namespace earth_curvature
    {
        const float EARTH_RADIUS = 6371.0f;

        /*
        This was mostly based off the following document :
        https://web.archive.org/web/20200110090856if_/http://ceeserver.cee.cornell.edu:80/wdp2/cee6150/Monograph/615_04_GeomCorrect_rev01.pdf
        */
        template <typename T>
        Image<T> correct_earth_curvature(Image<T> &image, float satellite_height, float swath, float resolution_km)
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

            Image<T> output_image(corrected_width, image.height(), image.channels()); // Allocate output image

            for (int channel = 0; channel < image.channels(); channel++)
            {
                int channel_offset = channel * (image.width() * image.height());
                int channel_offset_output = channel * (output_image.width() * output_image.height());

                // Process each row
                for (int row = 0; row < image.height(); row++)
                {
                    for (int i = 0; i < corrected_width; i++)
                    {
                        int pixel_to_use = correction_factors[i];                                                                                     // Input pixel to use, will get rounder automatically
                        output_image[channel_offset_output + row * corrected_width + i] = image[channel_offset + row * image.width() + pixel_to_use]; // Copy over that pixel!
                    }
                }
            }

            // Maybe we could do some more smoothing after the fact?
            // Will see later.

            delete[] correction_factors;

            return output_image;
        }

        template Image<uint8_t> correct_earth_curvature<uint8_t>(Image<uint8_t> &, float, float, float);
        template Image<uint16_t> correct_earth_curvature<uint16_t>(Image<uint16_t> &, float, float, float);
    }
}