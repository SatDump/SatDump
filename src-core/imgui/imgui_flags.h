#pragma once

// Has to be cast otherwise the compiler will complain...
#define NOWINDOW_FLAGS (long int)ImGuiWindowFlags_NoMove | (long int)ImGuiWindowFlags_NoCollapse /*| ImGuiWindowFlags_NoTitleBar*/ | (long int)ImGuiWindowFlags_NoResize | (long int)ImGuiWindowFlags_NoBackground | (long int)ImGuiWindowFlags_NoBringToFrontOnFocus