#pragma once

#include "json.hpp"

nlohmann::json loadJsonFile(std::string path);
void saveJsonFile(std::string path, nlohmann::json j);
