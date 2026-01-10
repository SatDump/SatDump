#include "logger.h"
#include <cstdint>
#include <vector>

/**
 * @file majority_law.h
 * @brief Functions to perform majority law between arrays or vectors of bytes
 */

namespace satdump
{
    std::vector<unsigned char> majority_law_vec(std::vector<std::vector<unsigned char>> input);

    /**
     * @brief Applies majority law between N arrays on the bit level
     * Defined here because cpp is stupid and needs the template definition in the header file
     *
     * @param input A 2D array that contains N [array_count] arrays with M [byte_count] bytes each
     * @param array_count The amount of arrays in the input
     * @param byte_count The count of bytes to apply the majority law to for each array
     * @param output Pointer to the output array to write to
     */
    template <size_t array_count, size_t byte_count>
    void majority_law(const uint8_t (&input)[array_count][byte_count], uint8_t *output)
    {
        if (array_count < 1)
        {
            logger->error("Majority law was attempted with no elements!");
            return;
        }

        for (int cur_byte_position = 0; cur_byte_position != byte_count; cur_byte_position++)
        {
            unsigned char output_byte = 0;

            for (int bit = 7; bit >= 0; bit--)
            {
                int zero_votes = 0;
                int one_votes = 0;

                // Gets the bit value from each element
                for (int cur_array = 0; cur_array < array_count; cur_array++)
                {
                    unsigned char byte = input[cur_array][cur_byte_position];

                    if (((byte >> bit) & 1) == 0)
                    {
                        zero_votes++;
                    }
                    else
                    {
                        one_votes++;
                    }
                }

                // Push new bit
                output_byte = output_byte << 1;

                // Set the new bit accordingly
                if (one_votes >= zero_votes)
                {
                    output_byte |= 1;
                }
            }

            // Add to output
            output[cur_byte_position] = output_byte;
        }
        return;
    }
} // namespace satdump
