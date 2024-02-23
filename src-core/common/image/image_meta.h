#pragma once

#include "image.h"
#include "nlohmann/json.hpp"

namespace image
{
    template <typename T>
    bool has_metadata(Image<T> &img)
    {
        return img.metadata_obj != nullptr;
    }

    template <typename T>
    void set_metadata(Image<T> &img, nlohmann::json metadata)
    {
        img.metadata_obj = (nlohmann::json *)new nlohmann::json();
        *((nlohmann::json *)img.metadata_obj) = metadata;
    }

    template <typename T>
    nlohmann::json get_metadata(const Image<T> &img)
    {
        if (img.metadata_obj != nullptr)
            return *((nlohmann::json *)img.metadata_obj);
        else
            return nlohmann::json();
    }
}