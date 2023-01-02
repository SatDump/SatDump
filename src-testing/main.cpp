/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

bool equation_contains(std::string init, std::string match)
{
    size_t pos = init.find(match);
    if (pos != std::string::npos)
    {
        std::string final_ex;
        while (pos < init.size())
        {
            char v = init[pos];
            if (v < 48 || v > 57)
                if (v < 65 || v > 90)
                    if (v < 97 || v > 122)
                        break;
            final_ex += v;
            pos++;
        }

        if (match == final_ex)
            return true;
    }

    return false;
}

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::vector<int> channel_indexes;
    std::vector<std::string> channel_numbers;

    std::string str_to_find_channels = "ch1, ch2, ch3";

    for (int i = 0; i < 40; i++)
    {
        // auto img = product.images[i];
        std::string equ_str = "ch" + std::to_string(i + 1); // img.channel_name;

        if (equation_contains(str_to_find_channels, equ_str))
        {
            channel_indexes.push_back(i);
            // channel_numbers.push_back(img.channel_name);
            // images_obj.push_back(img.image);
            // offsets.emplace(img.channel_name, img.offset_x);
            logger->debug("Composite needs channel {:s}", equ_str);

            // if (max_width_used < (int)img.image.width())
            //     max_width_used = img.image.width();

            // if (min_offset > img.offset_x)
            //     min_offset = img.offset_x;
        }
    }
}
