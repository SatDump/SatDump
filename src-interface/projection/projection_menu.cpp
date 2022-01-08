#include "projection_menu.h"

#include "imgui/imgui.h"
#include "global.h"
#include "logger.h"
#include "projection.h"
#include "settingsui.h"
#include "main_ui.h"
#include "findgeoref.h"
#include "imgui/file_selection.h"
#include "overlay.h"

namespace projection
{
    int output_width = 3000;
    int output_height = 3000;

    char overlay_image[1000];
    char overlay_georef[1000];

    void initProjectionMenu()
    {
        std::fill(overlay_image, &overlay_image[1000], 0);
        std::fill(overlay_georef, &overlay_georef[1000], 0);
    }

    void renderProjectionMenu(int /*wwidth*/, int /*wheight*/)
    {
        ImGui::CollapsingHeader("Reprojection");
        ImGui::InputInt("Projected Image Width ", &output_width, 100, 1000);
        ImGui::InputInt("Projected Image Height", &output_height, 100, 1000);
        if (ImGui::Button("Start##startreprojection"))
        {
            initProjection();
            satdumpUiStatus = PROJECTION;
        }
        ImGui::Text("This feature, that is projections and geo-referencing in SatDump in general \n"
                    "is still in a pretty early state, with probably a bunch of unknown bugs, \n"
                    "stuff I know about already and will fix before a full release, etc. Still, enjoy :-) \n");
        ImGui::Text(" - Aang23");
        ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "When filling in image sizes above, keep in mind Stereographic and Geostationnary \n"
                                                               "are 'square' outputs and will look weird otherwise, while equirectangular \n"
                                                               "looks best on a 2/1 height/width ratio. (eg, width 20000, height 10000) \n"
                                                               "Also, of course the resolution you set here will define how high in resolution\n"
                                                               "the final image will be. Increase if you want more!");

        ImGui::Spacing();
        ImGui::CollapsingHeader("Overlay");
        ImGui::InputText("Image ", overlay_image, 1000);
        if (ImGui::Button("Select Image"))
        {
            logger->debug("Opening file dialog");

            std::string input_file = selectInputFileDialog("Open input image", ".", {".*"});

            if (input_file.size() > 0)
            {
                logger->debug("Dir " + input_file);
                std::fill(overlay_image, &overlay_image[1000], 0);
                std::memcpy(overlay_image, input_file.c_str(), input_file.length());

                std::optional<std::string> georef = findGeoRefForImage(input_file);
                if (georef.has_value())
                {
                    std::fill(overlay_georef, &overlay_georef[1000], 0);
                    std::memcpy(overlay_georef, georef.value().c_str(), georef.value().length());
                }
            }
        }

        ImGui::InputText("Georef", overlay_georef, 1000);
        if (ImGui::Button("Select Georef"))
        {
            logger->debug("Opening file dialog");
            std::string input_file = selectInputFileDialog("Open input georef", ".", {".georef"});

            if (input_file.size() > 0)
            {
                logger->debug("Dir " + input_file);
                std::fill(overlay_georef, &overlay_georef[1000], 0);
                std::memcpy(overlay_georef, input_file.c_str(), input_file.length());
            }
        }
        if (ImGui::Button("Start##startoverlay"))
        {
            projection_overlay::initOverlay();
            satdumpUiStatus = OVERLAY;
        }
        ImGui::Text("This overlay-applying tool is temporary, and will be replaced in a \n"
                    "future release. \n"
                    "Expect a massive rework of both of those tools soon enough!\n");
        ImGui::Text(" - Aang23");
    }
};