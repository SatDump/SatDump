#pragma once
#include "nlohmann/json.hpp"

namespace widgets
{
	template <typename T>
	void JSONTreeEditor(T &json, const char *id, bool allow_add = true);

	bool JSONTableEditor(nlohmann::json &json, const char* id);
}