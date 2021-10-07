#include "projection_menu.h"

#include "imgui/imgui.h"
#include "global.h"
#include "logger.h"
#include "projection.h"
#include "settingsui.h"
#include "main_ui.h"

namespace projection
{
    int output_width = 3000;
    int output_height = 3000;

    void initProjectionMenu()
    {
    }

    void renderProjectionMenu(int /*wwidth*/, int /*wheight*/)
    {
        ImGui::InputInt("Projected Image Width ", &output_width, 100, 1000);
        ImGui::InputInt("Projected Image Height", &output_height, 100, 1000);
        if (ImGui::Button("Start"))
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
    }
};