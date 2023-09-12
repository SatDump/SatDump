#pragma once

#include "common/rimgui.h"
#include <mutex>

extern std::mutex gui_feedback_mtx;
extern RImGui::RImGui gui_local;
extern std::vector<RImGui::UiElem> last_draw_feedback;
extern uint64_t last_samplerate;

void sourceGuiThread();