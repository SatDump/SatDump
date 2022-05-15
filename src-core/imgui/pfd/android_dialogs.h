#pragma once

#include <string>

#ifdef __ANDROID__
std::string show_select_file_dialog();
std::string get_select_file_dialog_result();

std::string show_select_directory_dialog();
std::string get_select_directory_dialog_result();
#endif