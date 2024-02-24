#include "image_meta.h"

namespace image
{
    template <typename T>
    void Image<T>::copy_meta(const Image<T> &img)
    {
        if (img.metadata_obj != nullptr)
        {
            set_metadata(*this, get_metadata(img));
            printf("META SET!\n");
        }
    }

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}