#include "logger.h"
#include <vector>

namespace satdump
{
    /**
     * @brief Applies majority law between input elements on a bit level, returns a vector of the corrected value
     *
     * @param input A vector of vectors of bytes to apply majority law between. ALL ELEMENT VECTORS SHOULD BE THE SAME SIZE!!!
     * @return std::vector<unsigned char>
     */
    std::vector<unsigned char> majority_law_vec(std::vector<std::vector<unsigned char>> input)
    {
        std::vector<unsigned char> output;
        int zero_votes = 0;
        int one_votes = 0;
        int stop_byte_count = input[0].size();
        ;

        if (stop_byte_count == 0)
        {
            logger->error("Majority law was attempted with an empty vector!");
            return output;
        }

        for (int cur_byte_position = 0; cur_byte_position != stop_byte_count; cur_byte_position++)
        {
            unsigned char output_byte = 0;

            for (int bit = 7; bit >= 0; bit--)
            {
                // Gets the bit value from each element
                for (int element = 0; element < input.size(); element++)
                {
                    unsigned char byte = input[element][cur_byte_position];

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

                if (one_votes >= zero_votes)
                {
                    // If it is meant to be one, set it to one
                    output_byte |= 1;
                }

                // Reset the votes for the next bit
                one_votes = 0;
                zero_votes = 0;
            }
            // Add to output
            output.push_back(output_byte);
        }
        return output;
    }
} // namespace satdump