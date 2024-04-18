#pragma once

#include <string>
#include "common/image/image.h"
#include "lrit_file.h"

namespace lrit
{
    template <typename T>
    class LRITProductizer
    {
    private:
        bool should_sweep_x;
        std::string instrument_id = "ukn";

    public:
        LRITProductizer(std::string instrument_id, bool sweep_x);
        ~LRITProductizer();

        void saveImage(image::Image<T> img,
                       std::string directory,
                       std::string satellite,
                       std::string satshort,
                       std::string channel,
                       time_t timestamp,
                       std::string region = "",
                       lrit::ImageNavigationRecord *navigationHdr = nullptr);
    };
}