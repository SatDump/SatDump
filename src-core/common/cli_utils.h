#pragma once

#include <typeindex>
#include <optional>
#include "nlohmann/json.hpp"

/*
Parse command flags :

Flags :
 --flag value
This will parse the value either as a string or number.

Switches :
 --switch
This will set that parameter to true.
*/
nlohmann::json parse_common_flags(int argc, char *argv[], std::map<std::string, std::optional<std::type_index>> strong_types = {});