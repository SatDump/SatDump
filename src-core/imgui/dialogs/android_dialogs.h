#pragma once

#include <string>

// TODOREWORK bind this via backend::
#ifdef __ANDROID__
void show_select_file_dialog();
std::string get_select_file_dialog_result();

void show_select_directory_dialog();
std::string get_select_directory_dialog_result();
#endif