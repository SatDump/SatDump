#pragma once
#include "nlohmann/json.hpp"

namespace widgets
{
	void JSONEditor(nlohmann::ordered_json &json);
}