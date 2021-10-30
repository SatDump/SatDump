#include "crop.h"
#include "logger.h"

namespace goes
{
    namespace gvar
    {
        cimg_library::CImg<unsigned short> cropIR(cimg_library::CImg<unsigned short> input)
        {
            cimg_library::CImg<unsigned short> output(4749, input.height(), 1, input.spectrum());

            if (input.width() == 5206)
                output.draw_image(0, 0, 0, 0, input);
            else if (input.width() == 5209)
                output.draw_image(-463, 0, 0, 0, input);
            else
            {
                logger->warn("Wrong IR image size (" + std::to_string(input.width()) + "), it will not be cropped");
                return input;
            }

            return output;
        }

        cimg_library::CImg<unsigned short> cropVIS(cimg_library::CImg<unsigned short> input)
        {
            cimg_library::CImg<unsigned short> output(18990, input.height(), 1, input.spectrum());

            if (input.width() == 20824)
                output.draw_image(0, 0, 0, 0, input);
            else if (input.width() == 20836)
                output.draw_image(-1852, 0, 0, 0, input);
            else
            {
                logger->warn("Wrong VIS image size (" + std::to_string(input.width()) + "), it will not be cropped");
                return input;
            }

            return output;
        }
    };
};