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
            available_presets.push_back(loadJsonFile(resources::getResourcePath("pipeline_cfgs/AVHRR_MHS_Mix.json")));
            available_presets.push_back(loadJsonFile(resources::getResourcePath("pipeline_cfgs/MSUGS_Full_Disk.json")));
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

            if (ImGui::BeginTabItem("Presets"))
            {
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
                    run();

                ImGui::EndTabItem();
            }

            if (processor && ImGui::BeginTabItem("Edit"))
            {
                if (processor)
                    processor->renderUI();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        void DatasetProductHandler::run()
        {
            if (processor->can_process())
                processor->process();
        }
    }
}