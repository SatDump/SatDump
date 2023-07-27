#include "tracking_widget.h"
#include "common/utils.h"
#include "logger.h"
#include "imgui/imgui.h"

#include "rotcl_handler.h"

namespace satdump
{
    void TrackingWidget::rotatorth_run()
    {
        while (rotatorth_should_run)
        {
            rotator_handler_mtx.lock();
            if (rotator_handler->is_connected())
            {
                // logger->info("Rot update!");

                if (rotator_handler->get_pos(&current_rotator_az, &current_rotator_el) != RotatorHandler::ROT_ERROR_OK)
                    logger->error("Error getting rotator position!");

                if (rotator_engaged)
                {
                    if (rotator_tracking)
                    {
                        if (current_el > 0)
                        {
                            current_req_rotator_az = current_az;
                            current_req_rotator_el = current_el;
                        }
                        else
                        {
                            current_req_rotator_az = next_aos_az;
                            current_req_rotator_el = next_aos_el;
                        }
                    }

                    if (current_req_rotator_el < 0)
                        current_req_rotator_el = 0;

                    if (current_reql_rotator_az != current_req_rotator_az || current_reql_rotator_el != current_req_rotator_el)
                        if (rotator_handler->set_pos(current_req_rotator_az, current_req_rotator_el) != RotatorHandler::ROT_ERROR_OK)
                            logger->error("Error setting rotator position %f %f!", current_req_rotator_az, current_req_rotator_el);

                    current_reql_rotator_az = current_req_rotator_az;
                    current_reql_rotator_el = current_req_rotator_el;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(uint64_t(rotator_update_period * 1e3)));
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            rotator_handler_mtx.unlock();
        }
    }

    void TrackingWidget::renderRotatorStatus()
    {
        if (ImGui::BeginTable("##trackingwidgettable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Rot Az");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Rot El");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::InputFloat("##Rot Az", &current_req_rotator_az);
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##Rot El", &current_req_rotator_el);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%.3f", current_rotator_az);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", current_rotator_el);

            ImGui::EndTable();
        }

        ImGui::Checkbox("Engage", &rotator_engaged);
        ImGui::SameLine();
        ImGui::Checkbox("Track", &rotator_tracking);
        ImGui::SameLine();
        if (ImGui::Button("Cfg"))
            show_window_config = true;

        ImGui::Separator();

        if (rotator_handler->is_connected())
            style::beginDisabled();
        if (ImGui::Combo("Type##rotatortype", &selected_rotator_handler, "Rotctl\0"
                                                                         "PstRotator (Untested)\0"))
        {
            rotator_handler_mtx.lock();
            if (selected_rotator_handler == 0)
                rotator_handler = std::make_shared<RotctlHandler>();
            rotator_handler_mtx.unlock();
        }
        if (rotator_handler->is_connected())
            style::endDisabled();

        rotator_handler->render();
    }
}
