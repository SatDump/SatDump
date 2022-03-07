#pragma once

#include "json.hpp"

nlohmann::ordered_json loadJsonFile(std::string path);
void saveJsonFile(std::string path, nlohmann::ordered_json j);
