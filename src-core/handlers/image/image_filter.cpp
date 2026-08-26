#include "image_filter.h"
#include "i18n.h"
#include "image/adaptive_despeckle.h"
#include "image/adaptive_despeckle_json.h"
#include "image/brightness_contrast.h"
#include "image/hue_saturation.h"
#include "image/hue_saturation_json.h"
#include "image/image.h"
#include "image/image_background.h"
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

            void set(nlohmann::json p) { per_channel = p["per_channel"]; }
        };

        class BrightnessContrastConfigurator : public ImageFilterConfigurator
        {
        private:
            float brightness = 0;
            float contrast = 0;

        public:
            void draw()
            {
                ImGui::SliderFloat(_("Brightness"), &brightness, -2, 2);
                ImGui::SliderFloat(_("Contrast"), &contrast, -2, 2);
            }

            nlohmann::json get()
            {
                nlohmann::json p;
                p["brightness"] = brightness;
                p["contrast"] = contrast;
                return p;
            }

            void set(nlohmann::json p)
            {
                brightness = p["brightness"];
                contrast = p["contrast"];
            }
        };

        class HueSaturationConfigurator : public ImageFilterConfigurator
        {
        private:
            image::HueSaturation cfg;

        public:
            void draw()
            {
                for (int i = 0; i < 7; i++)
                {
                    std::string cname[] = {_("All"), _("Red"), _("Yellow"), _("Green"), _("Cyan"), _("Blue"), _("Magenta")};
                    float hue = cfg.hue[i] * 180.0;
                    float saturation = cfg.saturation[i] * 100.0;
                    float lightness = cfg.lightness[i] * 100.0;
                    ImGui::SliderFloat(std::string(cname[i] + _(" Hue")).c_str(), &hue, -180, 180);
                    ImGui::IsItemDeactivatedAfterEdit();
                    ImGui::SliderFloat(std::string(cname[i] + _(" Saturation")).c_str(), &saturation, -100, 100);
                    ImGui::IsItemDeactivatedAfterEdit();
                    ImGui::SliderFloat(std::string(cname[i] + _(" Lightness")).c_str(), &lightness, -100, 100);
                    ImGui::IsItemDeactivatedAfterEdit();
                    cfg.hue[i] = hue / 180.0;
                    cfg.saturation[i] = saturation / 100.0;
                    cfg.lightness[i] = lightness / 100.0;
                }

                float overlap = cfg.overlap;
                ImGui::SliderFloat(_("Overlap"), &overlap, -100.0, 100.0);
                cfg.overlap = overlap;
            }

            nlohmann::json get() { return cfg; }

            void set(nlohmann::json p) { cfg = p; }
        };

        class AdaptiveDespeckleConfigurator : public ImageFilterConfigurator
        {
        private:
            image::AdaptiveDespeckleConfig cfg;

        public:
            void draw()
            {
                if (ImGui::RadioButton(_("Adaptive"), cfg.filter_type == cfg.ADAPTIVE))
                    cfg.filter_type = cfg.ADAPTIVE;
                if (ImGui::RadioButton(_("Recursive"), cfg.filter_type == cfg.RECURSIVE))
                    cfg.filter_type = cfg.RECURSIVE;

                ImGui::InputInt(_("Radius"), &cfg.radius);
                if (cfg.radius < 0)
                    cfg.radius = 0;

                ImGui::SliderInt(_("Black Level"), &cfg.black_level, 0, 255);
                ImGui::SliderInt(_("White Level"), &cfg.white_level, 0, 255);
            }

            nlohmann::json get() { return cfg; }

            void set(nlohmann::json p) { cfg = p; }
        };

        std::map<std::string, ImageFilter> getImageFilters()
        {
            std::map<std::string, ImageFilter> filters;

            filters.insert({"equalize",
                            {_("Equalize"),
                             [](image::Image &img, nlohmann::json cfg)
                             {
                                 bool per_channel = cfg["per_channel"];
                                 image::equalize(img, per_channel);
                             }, //
                             []() { return std::make_shared<EqualizeConfigurator>(); }}});

            filters.insert({"normalize", {_("Normalize"), [](image::Image &img, nlohmann::json) { image::normalize(img); }}});

            filters.insert({"white_balance", {_("White Balance"), [](image::Image &img, nlohmann::json) { image::white_balance(img); }}});

            filters.insert({"invert", {_("Invert"), [](image::Image &img, nlohmann::json) { image::linear_invert(img); }}});

            filters.insert({"median_blur", {_("Median Blur"), [](image::Image &img, nlohmann::json) { image::median_blur(img); }}});

            filters.insert({"despeckle", {_("Despeckle"), [](image::Image &img, nlohmann::json) { image::kuwahara_filter(img); }}});

            filters.insert({"brightness_contrast",
                            {_("Brightness/contrast"),
                             [](image::Image &img, nlohmann::json cfg)
                             {
                                 float brightness = cfg["brightness"];
                                 float contrast = cfg["contrast"];
                                 image::brightness_contrast(img, brightness, contrast);
                             }, //
                             []() { return std::make_shared<BrightnessContrastConfigurator>(); }}});

            filters.insert({"remove_background", {_("Remove Background"), [](image::Image &img, nlohmann::json) { image::remove_background(img, nullptr); }}});

            filters.insert({"hue_saturation",
                            {_("Hue Saturation"), [](image::Image &img, nlohmann::json cfg) { image::hue_saturation(img, cfg); }, //
                             []() { return std::make_shared<HueSaturationConfigurator>(); }}});

            filters.insert({"adaptive_despeckle",
                            {_("Adaptive Despeckle"), [](image::Image &img, nlohmann::json cfg) { image::adaptive_despeckle(img, cfg); }, //
                             []() { return std::make_shared<AdaptiveDespeckleConfigurator>(); }}});

            return filters;
        }
    } // namespace handlers
} // namespace satdump