#pragma once

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
nlohmann::json parse_common_flags(int argc, char *argv[]);