#include "image_meta.h"

namespace image
{
    void Image::copy_meta(const Image &img)
    {
        if (img.metadata_obj != nullptr)
        {
            set_metadata(*this, get_metadata(img));
        }
    }

    bool has_metadata(Image &img)
    {
        return img.metadata_obj != nullptr;
    }

    void set_metadata(Image &img, nlohmann::json metadata)
    {
        img.metadata_obj = (nlohmann::json *)new nlohmann::json();
        *((nlohmann::json *)img.metadata_obj) = metadata;
    }

    nlohmann::json get_metadata(const Image &img)
    {
        if (img.metadata_obj != nullptr)
            return *((nlohmann::json *)img.metadata_obj);
        else
            return nlohmann::json();
    }

    void free_metadata(const Image &img)
    {
        if (img.metadata_obj != nullptr)
            delete ((nlohmann::json *)img.metadata_obj);
    }

    bool has_metadata_proj_cfg(Image &img)
    {
        return img.metadata_obj != nullptr && get_metadata(img).contains("proj_cfg");
    }

    void set_metadata_proj_cfg(Image &img, nlohmann::json proj_cfg)
    {
        if (img.metadata_obj == nullptr)
            img.metadata_obj = (nlohmann::json *)new nlohmann::json();

        satdump::reprojection::rescaleProjectionScalarsIfNeeded(proj_cfg, img.width(), img.height());

        (*((nlohmann::json *)img.metadata_obj))["proj_cfg"] = proj_cfg;
    }

    nlohmann::json get_metadata_proj_cfg(const Image &img)
    {
        if (img.metadata_obj != nullptr && get_metadata(img).contains("proj_cfg"))
            return (*((nlohmann::json *)img.metadata_obj))["proj_cfg"];
        else
            return nlohmann::json();
    }
}