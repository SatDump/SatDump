#include <vector>
#include <string>
#include "common/image/image.h"

namespace satdump
{
    template <typename T>
    std::string save_image_dialog(std::string default_name, std::string default_path, std::string window_title, image::Image<T>* image, std::string* default_ext);
}
