#include "xrit_channel_processor_render.h"
#include "core/style.h"
#include "imgui/imgui_image.h"
#include "xrit/processor/xrit_channel_processor.h"

namespace satdump
{
    namespace xrit
    {
        bool renderTabsFromProcessor(XRITChannelProcessor *p)
        {
            p->ui_img_mtx.lock();

            bool hasImage = false;

            for (auto &decMap : p->all_wip_images)
            {
                auto &dec = decMap.second;

                if (dec->textureID == 0)
                {
                    dec->textureID = makeImageTexture();
                    dec->textureBuffer = new uint32_t[dec->img_width * dec->img_height];
                    memset(dec->textureBuffer, 0, sizeof(uint32_t) * dec->img_width * dec->img_height);
                    dec->hasToUpdate = true;
                }

                if (dec->imageStatus != XRITChannelProcessor::IDLE)
                {
                    if (dec->hasToUpdate)
                    {
                        updateImageTexture(dec->textureID, dec->textureBuffer, dec->img_width, dec->img_height);
                        dec->hasToUpdate = false;
                    }

                    hasImage = true;

                    if (ImGui::BeginTabItem(std::string("Ch " + decMap.first).c_str()))
                    {
                        ImGui::Image((void *)(intptr_t)dec->textureID, {200 * ui_scale, 200 * ui_scale});
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                        if (dec->imageStatus == XRITChannelProcessor::SAVING)
                            ImGui::TextColored(style::theme.green, "Writing image...");
                        else if (dec->imageStatus == XRITChannelProcessor::RECEIVING)
                            ImGui::TextColored(style::theme.orange, "Receiving...");
                        else
                            ImGui::TextColored(style::theme.red, "Idle (Image)...");
                        ImGui::EndGroup();
                        ImGui::EndTabItem();
                    }
                }
            }

            p->ui_img_mtx.unlock();

            return hasImage;
        }

        void renderAllTabsFromProcessors(std::vector<XRITChannelProcessor *> ps)
        {
            if (ImGui::BeginTabBar("Images TabBar", ImGuiTabBarFlags_None))
            {
                bool hasImage = false;

                for (auto &p : ps)
                    hasImage |= satdump::xrit::renderTabsFromProcessor(p);

                if (!hasImage) // Add empty tab if there is no image yet
                {
                    if (ImGui::BeginTabItem("No image yet"))
                    {
                        ImGui::Dummy({200 * ui_scale, 200 * ui_scale});
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                        ImGui::TextColored(style::theme.red, "Idle (Image)...");
                        ImGui::EndGroup();
                        ImGui::EndTabItem();
                    }
                }

                ImGui::EndTabBar();
            }
        }

        void renderAllTabsFromProcessors(std::map<std::string, std::shared_ptr<satdump::xrit::XRITChannelProcessor>> &all_processors)
        {
            std::vector<XRITChannelProcessor *> ps;
            for (auto pp : all_processors)
                ps.push_back(pp.second.get());
            renderAllTabsFromProcessors(ps);
        }
    } // namespace xrit
} // namespace satdump