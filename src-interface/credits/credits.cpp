#include "credits.h"
#include "imgui/imgui.h"

namespace credits
{
    void renderCredits(int /*wwidth*/, int /*wheight*/)
    {
        ImGui::SetCursorPos({50, 100});
        ImGui::BeginGroup();

        ImGui::BeginGroup();
        ImGui::Text("Libraries :");
        ImGui::BulletText("libfmt");
        ImGui::BulletText("spdlog");
        ImGui::BulletText("libsathelper");
        ImGui::BulletText("libaec");
        ImGui::BulletText("libbzip");
        ImGui::BulletText("libpng");
        ImGui::BulletText("libjpeg");
        ImGui::BulletText("libfftw3");
        ImGui::BulletText("libcorrect");
        ImGui::BulletText("libairspy");
        ImGui::BulletText("librtlsdr");
        ImGui::BulletText("libhackrf");
        ImGui::BulletText("libpredict");
        ImGui::BulletText("miniz");
        ImGui::BulletText("tinyfiledialogs");
        ImGui::BulletText("ImGui");
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("Contributors :");
        ImGui::BulletText("Tomi HA6NAB");
        ImGui::BulletText("ZbychuButItWasTaken");
        ImGui::BulletText("Arved MØKDS");
        ImGui::BulletText("Ryzerth");
        ImGui::BulletText("LazzSnazz");
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("Others :");
        ImGui::BulletText("GNU Radio");
        ImGui::BulletText("OpenSatellite Project");
        ImGui::BulletText("goestools");
        ImGui::BulletText("Martin Blaho");
        ImGui::EndGroup();

        ImGui::EndGroup();
    }
}