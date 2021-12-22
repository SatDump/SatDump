#include "crop.h"
#include "logger.h"

namespace goes
{
    namespace gvar
    {
        template <typename T>
        image::Image<T> cropIR(image::Image<T> input)
        {
            image::Image<T> output(4749, input.height(), input.channels());

            if (input.width() == 5206)
                output.draw_image(0, input, 0, 0);
            else if (input.width() == 5209)
                output.draw_image(0, input, -463, 0);
            else
            {
                logger->warn("Wrong IR image size (" + std::to_string(input.width()) + "), it will not be cropped");
                return input;
            }

            return output;
        }

        template <typename T>
        image::Image<T> cropVIS(image::Image<T> input)
        {
            image::Image<T> output(18990, input.height(), input.channels());

            if (input.width() == 20824)
                output.draw_image(0, input, 0, 0);
            else if (input.width() == 20836)
                output.draw_image(0, input, -1852, 0);
            else
            {
                logger->warn("Wrong VIS image size (" + std::to_string(input.width()) + "), it will not be cropped");
                return input;
            }

            return output;
        }

        template image::Image<uint8_t> cropVIS(image::Image<uint8_t> input);
        template image::Image<uint16_t> cropVIS(image::Image<uint16_t> input);

        template image::Image<uint8_t> cropIR(image::Image<uint8_t> input);
        template image::Image<uint16_t> cropIR(image::Image<uint16_t> input);
    };
};