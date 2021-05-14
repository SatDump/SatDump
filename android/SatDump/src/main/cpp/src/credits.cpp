#include "credits.h"
#include "imgui/imgui.h"

void renderCredits(int wwidth, int wheight)
{
    ImGui::SetCursorPos({(float)wwidth / 2 - 250, (float)wheight / 2 - 90});
    ImGui::BeginGroup();

    ImGui::BeginGroup();
    ImGui::Text("Libraries :");
    ImGui::BulletText("libfmt");
    ImGui::BulletText("spdlog");
    ImGui::BulletText("libsathelper");
    ImGui::BulletText("libaec");
    ImGui::BulletText("libpng");
    ImGui::BulletText("libjpeg");
    ImGui::BulletText("libfftw3");
    ImGui::BulletText("libcorrect");
    ImGui::BulletText("libairspy");
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
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Others :");
    ImGui::BulletText("GNU Radio");
    ImGui::BulletText("OpenSatellite Project");
    ImGui::BulletText("Martin Blaho");
    ImGui::EndGroup();

    ImGui::EndGroup();
}