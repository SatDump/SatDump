#include "image_filter.h"
#include "i18n.h"
#include "image/image.h"
#include "image/processing.h"
#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace handlers
    {
        class EqualizeConfigurator : public ImageFilterConfigurator
        {
        private:
            bool per_channel = false;

        public:
            void draw() { ImGui::Checkbox("Per Channel", &per_channel); }

            nlohmann::json get()
            {
                nlohmann::json p;
                p["per_channel"] = per_channel;
                return p;
            }
        };

        std::map<std::string, ImageFilter> getImageFilters()
        {
            std::map<std::string, ImageFilter> filters;

            filters.insert({"equalize",
                            {_("Equalize"), [](image::Image &img, nlohmann::json) { image::equalize(img); }, //
                             []() { return std::make_shared<EqualizeConfigurator>(); }}});
            filters.insert({"normalize", {_("Normalize"), [](image::Image &img, nlohmann::json) { image::normalize(img); }}});

            return filters;
        }
    } // namespace handlers
} // namespace satdump