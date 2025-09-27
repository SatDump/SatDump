#include "logger.h"
#include <unordered_map>
#include <vector>

/**
 * @brief Applies majority law between input elements on a bit level, returns a vector of the corrected value
 *
 * @param input A vector of vectors of bytes to apply majority law between. ALL ELEMENT VECTORS SHOULD BE THE SAME SIZE!!!
 * @param big_endian If the data should be interpreted in big-endian order (back to front), returned output is in little-endian
 * @return std::vector<unsigned char>
 */
std::vector<unsigned char> majority_law(std::vector<std::vector<unsigned char>> input, bool big_endian)
{
    std::vector<unsigned char> output;
    std::unordered_map<int, int> votes = {{0, 0}, {1, 0}};
    int byte_count = input[0].size();
    int stop_byte_count = byte_count;

    if (byte_count == 0)
    {
        logger->error("Majority law was attempted with an empty vector!");
        return output;
    }

    int cur_byte_position = 0;
    if (big_endian)
    {
        cur_byte_position = byte_count - 1;
        stop_byte_count = 0;
    }

    while (cur_byte_position != stop_byte_count)
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
                    votes[0]++;
                }
                else
                {
                    votes[1]++;
                }
            }

            // Push new bit
            output_byte = output_byte << 1;

            if (votes[1] >= votes[0])
            {
                // If it is meant to be one, set it to one
                output_byte |= 1;
            }

            // Reset the votes for the next bit
            votes[0] = 0;
            votes[1] = 0;
        }
        // Add to output
        output.push_back(output_byte);

        // Big endian means we are going from the end
        if (big_endian)
        {
            cur_byte_position--;
        }
        else
        {
            cur_byte_position++;
        }
    }
    return output;
}
