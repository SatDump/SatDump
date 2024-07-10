#include "json_editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/module.h"

namespace widgets
{
	void JSONEditor(nlohmann::ordered_json& json)
	{
		for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> jsonItem : json.items())
		{
			ImGui::PushID(jsonItem.key().c_str());
			if (jsonItem.value().is_object() || jsonItem.value().is_array())
			{
				if (ImGui::TreeNode(jsonItem.key().c_str()))
				{
					JSONEditor(jsonItem.value());
					ImGui::TreePop();
				}
			}
			else if (jsonItem.value().is_boolean())
			{
				ImGui::BulletText("%s", jsonItem.key().c_str());
				ImGui::SameLine();
				bool val = jsonItem.value();
				if (ImGui::Checkbox(std::string("##" + jsonItem.key()).c_str(), &val))
					jsonItem.value() = val;
			}
			else if (jsonItem.value().is_number_integer() || jsonItem.value().is_number_unsigned())
			{
				ImGui::BulletText("%s", jsonItem.key().c_str());
				ImGui::SameLine();
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				int val = jsonItem.value();
				if (ImGui::InputInt(std::string("##" + jsonItem.key()).c_str(), &val))
					jsonItem.value() = val;
			}
			else if (jsonItem.value().is_number_float())
			{
				ImGui::BulletText("%s", jsonItem.key().c_str());
				ImGui::SameLine();
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				double val = jsonItem.value();
				if (ImGui::InputDouble(std::string("##" + jsonItem.key()).c_str(), &val))
					jsonItem.value() = val;
			}
			else if (jsonItem.value().is_string())
			{
				ImGui::BulletText("%s", jsonItem.key().c_str());
				ImGui::SameLine();
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				std::string val = jsonItem.value();
				if (ImGui::InputText(std::string("##" + jsonItem.key()).c_str(), &val))
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