#include <random>
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui_image.h"
#include "common/image/image.h"
#include "common/image/io.h"
#include "resources.h"
#include "core/style.h"
#include "core/backend.h"
#include "loader.h"
#include "const.h"

namespace satdump
{
    LoadingScreenSink::LoadingScreenSink()
    {
        const time_t timevalue = time(0);
        std::tm *timeConstant = gmtime(&timevalue);
        image::Image image;
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> check(1, 1000);
        loader_constant = ((timeConstant->tm_mon - 3) == 0 && (timeConstant->tm_mday - 1) == 0) ? (check(rng) != 42) : (check(rng) == 42);
        title = loader_constant ? satdump::loader_constant_title : "SatDump";
        slogan = loader_constant ? satdump::loader_constant_slogan : "General Purpose Satellite Data Processor";
        if (loader_constant)
            image::load_png(image, (uint8_t *)satdump::loader_constant_icon, sizeof(satdump::loader_constant_icon));
        else
            image::load_png(image, resources::getResourcePath("icon.png"));

        if (image.depth() != 8)
            image = image.to8bits();

        uint8_t *px = new uint8_t[image.width() * image.height() * 4];
        memset(px, 255, image.width() * image.height() * 4);

        if (image.channels() == 4)
        {
            for (int y = 0; y < (int)image.height(); y++)
                for (int x = 0; x < (int)image.width(); x++)
                    for (int c = 0; c < 4; c++)
                        px[image.width() * 4 * y + x * 4 + c] = image.get(c, image.width() * y + x);
        }
        else if (image.channels() == 3)
        {
            for (int y = 0; y < (int)image.height(); y++)
                for (int x = 0; x < (int)image.width(); x++)
                    for (int c = 0; c < 3; c++)
                        px[image.width() * 4 * y + x * 4 + c] = image.get(c, image.width() * y + x);
        }

        image_texture = makeImageTexture(image.width(), image.height());
        updateImageTexture(image_texture, (uint32_t *)px, image.width(), image.height());
        backend::setIcon(px, image.width(), image.height());
        delete[] px;
        push_frame("Initializing");
    }

    LoadingScreenSink::~LoadingScreenSink()
    {
        // deleteImageTexture(image_texture);
    }

    void LoadingScreenSink::receive(slog::LogMsg log)
    {
        if (log.lvl == slog::LOG_INFO)
            push_frame(log.str);
    }

    void LoadingScreenSink::push_frame(std::string str)
    {
        std::pair<int, int> dims = backend::beginFrame();
        float scale = backend::device_scale;
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({(float)dims.first, (float)dims.second});
        ImGui::Begin("Loading Screen", nullptr, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);

        if (dims.first > dims.second)
        {
            ImVec2 reference_pos = {((float)dims.first * 0.5f) - (300 * scale), ((float)dims.second * 0.5f) - (125 * scale)};
            ImGui::SetCursorPos(reference_pos);
            ImGui::Image((void *)image_texture, ImVec2(200 * scale, 200 * scale));
            ImGui::SetCursorPos({reference_pos.x + (230 * scale), reference_pos.y + (40 * scale)});
            ImGui::PushFont(style::bigFont);
            ImGui::TextUnformatted(title.c_str());
            ImGui::PopFont();
            ImGui::SetCursorPos({reference_pos.x + (230 * scale), reference_pos.y + (87 * scale)});
            ImGui::TextUnformatted(slogan.c_str());
            ImGui::GetWindowDrawList()->AddLine({reference_pos.x + (230 * scale), reference_pos.y + (112 * scale)},
                                                {reference_pos.x + (490 * scale), reference_pos.y + (112 * scale)}, IM_COL32(155, 155, 155, 255));
            ImGui::SetCursorPos({reference_pos.x + (230 * scale), reference_pos.y + (120 * scale)});
        }
        else
        {
            ImGui::PushFont(style::bigFont);
            ImVec2 title_size = ImGui::CalcTextSize(title.c_str());
            ImGui::SetCursorPos({((float)dims.first / 2) - (75 * scale), ((float)dims.second / 2) - title_size.y - (90 * scale)});
            ImGui::Image((void *)image_texture, ImVec2(150 * scale, 150 * scale));
            ImGui::SetCursorPos({((float)dims.first / 2) - (title_size.x / 2), ((float)dims.second / 2) - title_size.y + (75 * scale)});
            ImGui::TextUnformatted(title.c_str());
            ImGui::PopFont();
            ImVec2 slogan_size = ImGui::CalcTextSize(slogan.c_str());
            ImGui::SetCursorPos({((float)dims.first / 2) - (slogan_size.x / 2), ((float)dims.second / 2) + (80 * scale)});
            ImGui::TextUnformatted(slogan.c_str());
            ImGui::GetWindowDrawList()->AddLine({((float)dims.first / 2) - (slogan_size.x / 2), ((float)dims.second / 2) + (90 * scale) + slogan_size.y},
                                                {((float)dims.first / 2) + (slogan_size.x / 2), ((float)dims.second / 2) + (90 * scale) + slogan_size.y}, IM_COL32(155, 155, 155, 255));
            ImGui::SetCursorPos({((float)dims.first / 2) - (ImGui::CalcTextSize(str.c_str()).x / 2), ((float)dims.second / 2) + (95 * scale) + slogan_size.y});
        }

        ImGui::TextDisabled("%s", str.c_str());
        ImGui::End();
        backend::endFrame();
    }
}
