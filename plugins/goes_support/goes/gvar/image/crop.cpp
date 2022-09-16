#include "crop.h"
#include "logger.h"

namespace goes
{
    namespace gvar
    {
        template <typename T>
        image::Image<T> cropIR(image::Image<T> input)
        {
            if (input.width() == 5206)
                input.crop(0, 4749);
            else if (input.width() == 5209)
                input.crop(463, 463 + 4749);
            else
            {
                logger->warn("Wrong IR image size (" + std::to_string(input.width()) + "), it will not be cropped");
                return input;
            }

            return input;
        }

        template <typename T>
        image::Image<T> cropVIS(image::Image<T> input)
        {
            if (input.width() == 20824)
                input.crop(0, 18990);
            else if (input.width() == 20836)
                input.crop(1852, 1852 + 18990);
            else
            {
                logger->warn("Wrong VIS image size (" + std::to_string(input.width()) + "), it will not be cropped");
                return input;
            }

            return input;
        }

        template image::Image<uint8_t> cropVIS(image::Image<uint8_t> input);
        template image::Image<uint16_t> cropVIS(image::Image<uint16_t> input);

        template image::Image<uint8_t> cropIR(image::Image<uint8_t> input);
        template image::Image<uint16_t> cropIR(image::Image<uint16_t> input);
    };
};