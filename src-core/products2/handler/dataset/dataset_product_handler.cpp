#include "dataset_product_handler.h"
#include "logger.h"

// TODOREWORK
#include "resources.h"
#include "nlohmann/json_utils.h"

#include "lua_processor.h"
#include "flowgraph_processor.h"

namespace satdump
{
    namespace viewer
    {
        DatasetProductHandler::DatasetProductHandler()
        {
            handler_tree_icon = "\uf70e";

            available_presets.push_back(loadJsonFile(resources::getResourcePath("pipeline_cfgs/AVHRR_MHS_Mix.json")));
            available_presets.push_back(loadJsonFile(resources::getResourcePath("pipeline_cfgs/MSUGS_Full_Disk.json")));
            available_presets.push_back(loadJsonFile(resources::getResourcePath("pipeline_cfgs/AVHRR_as_MHS.json")));
            available_presets.push_back(loadJsonFile(resources::getResourcePath("pipeline_cfgs/AVHRR_MCIR_Sample.json")));
        }

        DatasetProductHandler::~DatasetProductHandler()
        {
        }

        void DatasetProductHandler::drawMenu()
        {
        }

        void DatasetProductHandler::drawContents(ImVec2 win_size)
        {
            ImGui::BeginTabBar("##datasetproducttabbar");

            bool proc_now = is_processing;

            if (ImGui::BeginTabItem("Presets"))
            {
                if (proc_now)
                    style::beginDisabled();

                if (ImGui::BeginListBox("##processingpipelineslistbox"))
                {
                    for (auto &p : available_presets)
                    {
                        bool sel = current_cfg == p["name"].get<std::string>();
                        if (ImGui::Selectable(p["name"].get<std::string>().c_str(), &sel))
                        {
                            current_cfg = p["name"];
                            if (p["processor"].get<std::string>() == "lua_processor")
                                processor = std::make_unique<Lua_DatasetProductProcessor>(dataset_handler, this, p);
                            else if (p["processor"].get<std::string>() == "flowgraph_processor")
                                processor = std::make_unique<Flowgraph_DatasetProductProcessor>(dataset_handler, this, p);
                        }
                    }
                    ImGui::EndListBox();
                }

                if (processor)
                    processor->renderParams();

                if (ImGui::Button("Generate"))
                    asyncProcess();

                if (processor)
                    ImGui::SameLine();
                if (ImGui::Button("Get JSON"))
                {
                    std::string str = processor->getCfg().dump(4);
                    ImGui::SetClipboardText(str.c_str());
                }

                if (proc_now)
                    style::endDisabled();

                ImGui::EndTabItem();
            }

            if (processor && ImGui::BeginTabItem("Edit"))
            {
                if (proc_now)
                    style::beginDisabled();
                if (processor)
                    processor->renderUI();
                if (proc_now)
                    style::endDisabled();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        void DatasetProductHandler::do_process()
        {
            if (processor)
            {
                if (processor->can_process())
                    processor->process();
            }
            else
                logger->error("Invalid processor!\n");
        }
    }
}