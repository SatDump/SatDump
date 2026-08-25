#pragma once

#include "image/image.h"
#include "nlohmann/json.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace satdump
{
    namespace handlers
    {
        class ImageFilterConfigurator
        {
        public:
            std::string type;
            virtual void draw() = 0;
            virtual nlohmann::json get() = 0;
            virtual void set(nlohmann::json) = 0;
        };

        struct ImageFilter
        {
            std::string name;
            std::function<void(image::Image &, nlohmann::json)> perform;
            std::function<std::shared_ptr<ImageFilterConfigurator>()> configMenuGetter = []() { return nullptr; };
        };

        std::map<std::string, ImageFilter> getImageFilters();
    } // namespace handlers
} // namespace satdump