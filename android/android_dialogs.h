#pragma once

#include <string>

void show_select_file_dialog();
std::string get_select_file_dialog_result();

void show_select_directory_dialog();
std::string get_select_directory_dialog_result();

void show_select_filesave_dialog(std::string name);
std::string get_select_filesave_dialog_result();