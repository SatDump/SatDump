#pragma once
#include <string>

template <typename T>
std::string format_notated(T val, std::string units = "", int num_decimals = -1, bool can_go_below_one = true);
