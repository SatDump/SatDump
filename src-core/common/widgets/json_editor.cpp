#include "json_editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/module.h"

namespace widgets
{
	void JSONEditor(nlohmann::ordered_json& json)
	{
		for (auto &jsonItem : json.items())
		{
			ImGui::PushID(jsonItem.key().c_str());
			if (jsonItem.value().is_object())
			{
				bool nodeOpen = ImGui::TreeNode(jsonItem.key().c_str());
				ImGui::SameLine();
				ImGui::TextDisabled("%s", "[Object]");
				if (nodeOpen)
				{
					JSONEditor(jsonItem.value());
					ImGui::TreePop();
				}
			}
			else if (jsonItem.value().is_array())
			{
				bool nodeOpen = ImGui::TreeNode(std::string((json.is_array() ? "##" : "") + jsonItem.key()).c_str());
				ImGui::SameLine();
				ImGui::TextDisabled("%s", jsonItem.value().dump().c_str());
				if (nodeOpen)
				{
					JSONEditor(jsonItem.value());
					ImGui::TreePop();
				}
			}
			else if (jsonItem.value().is_boolean())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", jsonItem.key().c_str());
					ImGui::SameLine();
				}
				bool val = jsonItem.value();
				if (ImGui::Checkbox(std::string("##" + jsonItem.key()).c_str(), &val))
					jsonItem.value() = val;
			}
			else if (jsonItem.value().is_number_integer() || jsonItem.value().is_number_unsigned())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", jsonItem.key().c_str());
					ImGui::SameLine();
				}
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				int val = jsonItem.value();
				if (ImGui::InputInt(std::string("##" + jsonItem.key()).c_str(), &val))
					jsonItem.value() = val;
			}
			else if (jsonItem.value().is_number_float())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", jsonItem.key().c_str());
					ImGui::SameLine();
				}
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				double val = jsonItem.value();
				if (ImGui::InputDouble(std::string("##" + jsonItem.key()).c_str(), &val))
					jsonItem.value() = val;
			}
			else if (jsonItem.value().is_string())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", jsonItem.key().c_str());
					ImGui::SameLine();
				}
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				std::string val = jsonItem.value();
				if (val.find("\n") == std::string::npos ? ImGui::InputText(std::string("##" + jsonItem.key()).c_str(), &val) :
					ImGui::InputTextMultiline(std::string("##" + jsonItem.key()).c_str(), &val))
					jsonItem.value() = val;
			}

			ImGui::PopID();
		}

		if (ImGui::Button("Add...##NewItem"))
		{
			//TODO
		}
	}
}