#include <vector>
#include <string>
#include "common/image2/image.h"

namespace satdump
{
    std::string save_image_dialog(std::string default_name, std::string default_path, std::string window_title, image2::Image *image, std::string *default_ext);
}
