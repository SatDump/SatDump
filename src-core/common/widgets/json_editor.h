#pragma once
#include "nlohmann/json.hpp"

namespace widgets
{
	void JSONEditor(nlohmann::ordered_json &json, const char* id, bool allow_add = true);
}