#pragma once

#include "image.h"
#include "nlohmann/json.hpp"
#include "common/projection/reprojector.h"

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

    template <typename T>
    bool has_metadata_proj_cfg(Image<T> &img)
    {
        return img.metadata_obj != nullptr && get_metadata(img).contains("proj_cfg");
    }

    template <typename T>
    void set_metadata_proj_cfg(Image<T> &img, nlohmann::json proj_cfg)
    {
        if (img.metadata_obj == nullptr)
            img.metadata_obj = (nlohmann::json *)new nlohmann::json();

        satdump::reprojection::rescaleProjectionScalarsIfNeeded(proj_cfg, img.width(), img.height());

        (*((nlohmann::json *)img.metadata_obj))["proj_cfg"] = proj_cfg;
    }

    template <typename T>
    nlohmann::json get_metadata_proj_cfg(const Image<T> &img)
    {
        if (img.metadata_obj != nullptr && get_metadata(img).contains("proj_cfg"))
            return (*((nlohmann::json *)img.metadata_obj))["proj_cfg"];
        else
            return nlohmann::json();
    }
}