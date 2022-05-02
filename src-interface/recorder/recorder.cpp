#include "recorder.h"
#include "core/config.h"
#include "common/utils.h"

namespace satdump
{
    RecorderApplication::RecorderApplication()
        : Application("recorder")
    {
    }

    RecorderApplication::~RecorderApplication()
    {
    }

    void RecorderApplication::drawUI()
    {
        ImVec2 recorder_size = ImGui::GetContentRegionAvail();
        ImGui::BeginGroup();
        ImGui::Text("Hello there! TODO");
        ImGui::EndGroup();
    }
};