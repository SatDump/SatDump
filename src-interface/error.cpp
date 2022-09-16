#include "error.h"
#include "imgui/imgui.h"
#include <string>

namespace satdump
{
    namespace error
    {
        bool hasError = false;
        std::string error_title;
        std::string error_message;

        void set_error(std::string title, std::string description)
        {
            error_title = title;
            error_message = description;
            hasError = true;
        }

        void render()
        {
            ImGui::Begin(error_title.c_str(), &hasError);
            ImGui::TextColored(ImColor(255, 0, 0), "%s", error_message.c_str());
            if (ImGui::Button("Close"))
                hasError = false;
            ImGui::End();
        }
    }
}