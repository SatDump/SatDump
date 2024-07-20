#include "json_editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/module.h"

namespace widgets
{
	[[nodiscard]] inline bool DeleteButton()
	{
		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPos().y - 2 * ui_scale);
		ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		bool ret = ImGui::Button(u8"\uf00d");
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(2);
		return ret;
	}

	void JSONEditor(nlohmann::ordered_json& json, const char* id, bool allow_add)
	{
		ImGui::PushID(id);
		int array_index = 0;
		bool delete_item = false;

		for (auto jsonItem = json.begin(); jsonItem != json.end(); )
		{
			std::string this_key;
			if (json.is_array())
				this_key = std::to_string(array_index++);
			else
				this_key = jsonItem.key();

			if(!jsonItem.value().is_object() && !jsonItem.value().is_array())
				ImGui::PushID(this_key.c_str());

			if (jsonItem.value().is_object())
			{
				bool nodeOpen = ImGui::TreeNode(std::string((json.is_array() ? "##" : "") + this_key).c_str());
				ImGui::SameLine();
				ImGui::TextDisabled("%s", "[Object]");
				delete_item = DeleteButton();
				if (nodeOpen)
				{
					JSONEditor(jsonItem.value(), this_key.c_str());
					ImGui::TreePop();
				}
			}
			else if (jsonItem.value().is_array())
			{
				bool nodeOpen = ImGui::TreeNode(std::string((json.is_array() ? "##" : "") + this_key).c_str());
				ImGui::SameLine();
				ImGui::TextDisabled("%s", jsonItem.value().dump().c_str());
				delete_item = DeleteButton();
				if (nodeOpen)
				{
					JSONEditor(jsonItem.value(), this_key.c_str());
					ImGui::TreePop();
				}
			}
			else if (jsonItem.value().is_boolean())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", this_key.c_str());
					ImGui::SameLine();
				}
				bool val = jsonItem.value();
				if (ImGui::Checkbox(std::string("##" + this_key).c_str(), &val))
					jsonItem.value() = val;
				delete_item = DeleteButton();
			}
			else if (jsonItem.value().is_number_integer() || jsonItem.value().is_number_unsigned())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", this_key.c_str());
					ImGui::SameLine();
				}
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				int val = jsonItem.value();
				if (ImGui::InputInt(std::string("##" + this_key).c_str(), &val))
					jsonItem.value() = val;
				delete_item = DeleteButton();
			}
			else if (jsonItem.value().is_number_float())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", this_key.c_str());
					ImGui::SameLine();
				}
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				double val = jsonItem.value();
				if (ImGui::InputDouble(std::string("##" + this_key).c_str(), &val))
					jsonItem.value() = val;
				delete_item = DeleteButton();
			}
			else if (jsonItem.value().is_string())
			{
				if (!json.is_array())
				{
					ImGui::BulletText("%s", this_key.c_str());
					ImGui::SameLine();
				}
				float next_width = (400 * ui_scale) - ImGui::GetCursorPosX();
				if (next_width > 0)
					ImGui::SetNextItemWidth(next_width);

				std::string val = jsonItem.value();
				if (val.find("\n") == std::string::npos ? ImGui::InputText(std::string("##" + this_key).c_str(), &val) :
					ImGui::InputTextMultiline(std::string("##" + this_key).c_str(), &val))
					jsonItem.value() = val;
				delete_item = DeleteButton();
			}

			if (!jsonItem.value().is_object() && !jsonItem.value().is_array())
				ImGui::PopID();

			if (delete_item)
			{
				jsonItem = json.erase(jsonItem);
				delete_item = false;
			}
			else
				jsonItem++;
		}

		if (allow_add && ImGui::Button("Add..."))
			ImGui::OpenPopup("Add Item");

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Add Item", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static std::string new_key = "new_item";
			if (json.is_object())
			{
				ImGui::TextUnformatted("Key:");
				ImGui::SameLine();
				ImGui::InputText("##label", &new_key, ImGuiInputTextFlags_AutoSelectAll);
			}

			static int selected = 0;
			ImGui::SameLine();
			ImGui::TextUnformatted("Type:");
			ImGui::SameLine();
			ImGui::Combo("##Type", &selected, "Object\0Array\0String\0Integer\0Double\0Boolean\0\0");
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::SetCursorPos({ (ImGui::GetContentRegionAvail().x / 2) - ((ImGui::CalcTextSize("Add").x + ImGui::CalcTextSize("Cancel").x +
				style.ItemSpacing.x + style.FramePadding.x * 4) / 2), ImGui::GetCursorPosY() + 5.0f * ui_scale});

			if (ImGui::Button("Add"))
			{
				if (json.is_object())
				{
					switch (selected)
					{
						case 0:
							json[new_key] = nlohmann::ordered_json::object();
							break;
						case 1:
							json[new_key] = nlohmann::ordered_json::array();
							break;
						case 2:
							json[new_key] = "";
							break;
						case 3:
							json[new_key] = 0;
							break;
						case 4:
							json[new_key] = 0.0;
							break;
						case 5:
							json[new_key] = false;
					}
				}
				else
				{
					switch (selected)
					{
						case 0:
							json.push_back(nlohmann::ordered_json::object());
							break;
						case 1:
							json.push_back(nlohmann::ordered_json::array());
							break;
						case 2:
							json.push_back("");
							break;
						case 3:
							json.push_back(0);
							break;
						case 4:
							json.push_back(0.0);
							break;
						case 5:
							json.push_back(false);
					}
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}
}