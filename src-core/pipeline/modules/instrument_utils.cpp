#include "instrument_utils.h"
#include "core/style.h"
#include "imgui/imgui.h"

// namespace satdump
// {
//     namespace pipeline
//     { TODOREWORK
void drawStatus(instrument_status_t status)
{
    if (status == DECODING)
        ImGui::TextColored(style::theme.yellow, "Decoding...");
    else if (status == PROCESSING)
        ImGui::TextColored(style::theme.magenta, "Processing...");
    else if (status == SAVING)
        ImGui::TextColored(style::theme.light_green, "Saving...");
    else if (status == DONE)
        ImGui::TextColored(style::theme.green, "Done");
    else
        ImGui::TextColored(style::theme.red, "Invalid!");
};
//     } // namespace pipeline
// } // namespace satdump