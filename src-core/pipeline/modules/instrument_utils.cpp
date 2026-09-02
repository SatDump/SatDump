#include "instrument_utils.h"
#include "core/style.h"
#include "i18n.h"
#include "imgui/imgui.h"

// namespace satdump
// {
//     namespace pipeline
//     { TODOREWORK
void drawStatus(instrument_status_t status)
{
    if (status == DECODING)
        ImGui::TextColored(style::theme.yellow, _("Decoding..."));
    else if (status == PROCESSING)
        ImGui::TextColored(style::theme.magenta, _("Processing..."));
    else if (status == SAVING)
        ImGui::TextColored(style::theme.light_green, _("Saving..."));
    else if (status == DONE)
        ImGui::TextColored(style::theme.green, _("Done"));
    else
        ImGui::TextColored(style::theme.red, _("Invalid!"));
};
//     } // namespace pipeline
// } // namespace satdump