#include "viewer.h"
// #include "image_handler.h"
// #include "radiation_handler.h"
// #include "scatterometer_handler.h"
#include "core/config.h"
#include "products/dataset.h"
#include "common/utils.h"
#include "resources.h"
#include "main_ui.h"

// void SelectableColor(ImU32 color) // Colors a cell in the table with the specified color in RGBA
//{
//     ImVec2 p_min = ImGui::GetItemRectMin();
//     ImVec2 p_max = ImGui::GetItemRectMax();
//     ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, color);
// }

#include "image_handler.h"

namespace satdump
{
    ViewerApplication2::ViewerApplication2()
        : Application("viewer")
    {
        std::string default_dir = config::main_cfg["satdump_directories"]["default_input_directory"]["value"].get<std::string>();
        select_dataset_products_dialog.setDefaultDir(default_dir);

        loadProductsInViewer("/home/alan/Downloads/SatDump_NEWPRODS/aws_pfm_cadu/STERNA_Dump/product.cbor");
        loadProductsInViewer("/home/alan/Downloads/SatDump_NEWPRODS/KMSS_24/KMSS_MSU100_1/product.cbor");
    }

    ViewerApplication2::~ViewerApplication2()
    {
    }

    void ViewerApplication2::loadDatasetInViewer(std::string path)
    {
        ProductDataSet dataset;
        dataset.load(path);

        std::string dataset_name = dataset.satellite_name + " " + timestamp_to_string(dataset.timestamp);

        int i = -1;
        bool contains = false;
        do
        {
            contains = false;
            std::string curr_name = ((i + 1) == 0 ? dataset_name : (dataset_name + " #" + std::to_string(i + 1)));
            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                if (products_and_handlers[i]->dataset_name == curr_name)
                    contains = true;
            i++;
        } while (contains);
        dataset_name = (i == 0 ? dataset_name : (dataset_name + " #" + std::to_string(i)));

        std::string pro_directory = std::filesystem::path(path).parent_path().string();
        for (std::string pro_path : dataset.products_list)
        {
            try
            {
                if (path.find("http") == 0)
                {
                    // Make sure the path is URL safe
                    std::ostringstream encodedUrl;
                    encodedUrl << std::hex << std::uppercase << std::setfill('0');
                    for (char &c : pro_path)
                    {
                        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
                            encodedUrl << c;
                        else
                            encodedUrl << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(c));
                    }
                    pro_path = encodedUrl.str();
                }
                loadProductsInViewer(pro_directory + "/" + pro_path, dataset_name);
            }
            catch (std::exception &e)
            {
                logger->error("Could not open " + pro_path + " in viewer! : %s", e.what());
            }
        }
    }

    void ViewerApplication2::loadProductsInViewer(std::string path, std::string dataset_name)
    {
        if (std::filesystem::exists(path) || path.find("http") == 0)
        {
            std::shared_ptr<products::Product> product = products::loadProduct(path);

            // Get instrument settings
            nlohmann::ordered_json instrument_viewer_settings;
            if (config::main_cfg["viewer"]["instruments"].contains(product->instrument_name))
                instrument_viewer_settings = config::main_cfg["viewer"]["instruments"][product->instrument_name];
            else
                logger->error("Unknown instrument : %s!", product->instrument_name.c_str());

            // Init Handler
            std::string handler_id;
            if (instrument_viewer_settings.contains("handler"))
                handler_id = instrument_viewer_settings["handler"].get<std::string>();
            else if (product->contents["type"] == "image")
                handler_id = "image_handler";
            else if (product->contents["type"] == "radiation")
                handler_id = "radiation_handler";
            else if (product->contents["type"] == "scatterometer")
                handler_id = "scatterometer_handler";
            logger->debug("Using handler %s for instrument %s", handler_id.c_str(), product->instrument_name.c_str());
            std::shared_ptr<ViewerHandler2> handler = viewer_handlers_registry2[handler_id]();

            handler->product = product.get();
            handler->instrument_cfg = instrument_viewer_settings;
            handler->init();

            if (dataset_name != "")
            {
                bool dataset_exists = false;
                for (std::string dataset_name2 : opened_datasets)
                    dataset_exists = dataset_exists | (dataset_name2 == dataset_name);

                if (!dataset_exists)
                    opened_datasets.push_back(dataset_name);
            }

            // Push products and handler
            product_handler_mutex.lock();
            products_and_handlers.push_back(std::make_shared<ProductsHandler2>(product, handler, dataset_name));
            product_handler_mutex.unlock();
        }
    }

    ImRect ViewerApplication2::renderHandler(ProductsHandler2 &ph, int index)
    {
        std::string label = ph.products->instrument_name;
        if (ph.handler->instrument_cfg.contains("name"))
            label = ph.handler->instrument_cfg["name"].get<std::string>();
        if (ph.products->has_product_source())
            label = ph.products->get_product_source() + " " + label;
        if (ph.products->has_product_timestamp())
            label = label + " " + timestamp_to_string(ph.products->get_product_timestamp());

        ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (index == current_handler_id ? ImGuiTreeNodeFlags_Selected : 0));
        if (ImGui::IsItemClicked())
            current_handler_id = index;

        if (index == current_handler_id && ph.dataset_name == "")
        { // Closing button
            ImGui::SameLine();
            ImGui::Text("  ");
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
            if (ImGui::SmallButton(std::string(u8"\uf00d##" + ph.dataset_name + label).c_str()))
            {
                logger->info("Closing products " + label);
                ph.marked_for_close = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
        }

        ImRect rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

        if (index == current_handler_id)
        {
            ImGui::TreePush(std::string("##HandlerTree" + std::to_string(current_handler_id)).c_str());
            products_and_handlers[current_handler_id]->handler->drawTreeMenu();
            ImGui::TreePop();
        }
        // ImGui::InputInt("Current Product", &current_handler_id);

        return rect;
    }

    void ViewerApplication2::drawPanel()
    {
        if (ImGui::BeginTabBar("Viewer Prob Tabbar", ImGuiTabBarFlags_None))
        {
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
            if (ImGui::BeginTabItem("Products###productsviewertab"))
            {
                if (current_selected_tab != 0)
                    current_selected_tab = 0;

                if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (std::string dataset_name : opened_datasets)
                    {
                        if (products_cnt_in_dataset(dataset_name))
                        {
                            ImGui::TreeNodeEx(dataset_name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                            ImGui::TreePush(std::string("##HandlerTree" + dataset_name).c_str());

                            { // Closing button
                                ImGui::SameLine();
                                ImGui::Text("  ");
                                ImGui::SameLine();

                                ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
                                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
                                if (ImGui::SmallButton(std::string(u8"\uf00d##dataset" + dataset_name).c_str()))
                                {
                                    logger->info("Closing datset " + dataset_name);
                                    for (int i = 0; i < (int)products_and_handlers.size(); i++)
                                        if (products_and_handlers[i]->dataset_name == dataset_name)
                                        {
                                            products_and_handlers[i]->marked_for_close = true;
                                        }
                                }
                                ImGui::PopStyleVar();
                                ImGui::PopStyleColor(2);
                            }

                            const ImColor TreeLineColor = ImColor(128, 128, 128, 255); // ImGui::GetColorU32(ImGuiCol_Text);
                            const float SmallOffsetX = 11.0f;                          // for now, a hardcoded value; should take into account tree indent size
                            ImDrawList *drawList = ImGui::GetWindowDrawList();

                            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
                            verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
                            ImVec2 verticalLineEnd = verticalLineStart;

                            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                            {
                                if (products_and_handlers[i]->dataset_name == dataset_name)
                                {
                                    const float HorizontalTreeLineSize = 8.0f * ui_scale;                 // chosen arbitrarily
                                    const ImRect childRect = renderHandler(*products_and_handlers[i], i); // RenderTree(child);
                                    const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
                                    drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
                                    verticalLineEnd.y = midpoint;
                                }
                            }

                            drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

                            ImGui::TreePop();
                        }
                    }

                    // Render unclassified
                    if (products_cnt_in_dataset(""))
                    {
                        ImGui::TreeNodeEx("Others", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        ImGui::TreePush("##HandlerTreeOthers");
                        for (int i = 0; i < (int)products_and_handlers.size(); i++)
                            if (products_and_handlers[i]->dataset_name == "")
                                renderHandler(*products_and_handlers[i], i);
                        ImGui::TreePop();
                    }

                    // Handle deletion if required
                    for (int i = 0; i < (int)products_and_handlers.size(); i++)
                    {
                        if (products_and_handlers[i]->marked_for_close)
                        {
                            product_handler_mutex.lock();
                            products_and_handlers.erase(products_and_handlers.begin() + i);
                            product_handler_mutex.unlock();
                            if (current_handler_id >= (int)products_and_handlers.size())
                                current_handler_id = 0;
                            break;
                        }
                    }

                    ImGui::Separator();
                    ImGui::Text("Load Dataset/Products :");
                    if (select_dataset_products_dialog.draw())
                    {
                        ui_thread_pool.push([this](int)
                                            {
                            try
                            {
                                std::string path = select_dataset_products_dialog.getPath();
                                if(std::filesystem::path(path).extension() == ".json")
                                    loadDatasetInViewer(path);
                                else if(std::filesystem::path(path).extension() == ".cbor")
                                    loadProductsInViewer(path);
                                else
                                    logger->error("Invalid file! Not products or dataset!");
                            }
                            catch (std::exception &e)
                            {
                                logger->error("Error opening dataset/products - %s", e.what());
                            } });
                    }

                    eventBus->fire_event<RenderLoadMenuElementsEvent>({});
                }

                if (products_and_handlers.size() > 0)
                    products_and_handlers[current_handler_id]->handler->drawMenu();

                ImGui::EndTabItem();
            }

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
            if (ImGui::BeginTabItem("Projections###projssviewertab"))
            {
                if (current_selected_tab != 1)
                    current_selected_tab = 1;
                //    drawProjectionPanel();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void ViewerApplication2::drawContent()
    {
    }

    void ViewerApplication2::drawUI()
    {
        ImVec2 viewer_size = ImGui::GetContentRegionAvail();

        if (ImGui::BeginTable("##wiever_table", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_None, viewer_size.x * panel_ratio);
            ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_None, viewer_size.x * (1.0f - panel_ratio));
            ImGui::TableNextColumn();

            float left_width = ImGui::GetColumnWidth(0);
            float right_width = viewer_size.x - left_width;
            if (left_width != last_width && last_width != -1)
                panel_ratio = left_width / viewer_size.x;
            last_width = left_width;

            ImGui::BeginChild("ViewerChildPanel", {left_width, float(viewer_size.y - 10)});
            drawPanel();
            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginGroup();
            if (current_selected_tab == 0)
            {
                if (products_and_handlers.size() > 0)
                    products_and_handlers[current_handler_id]->handler->drawContents({float(right_width - 4), float(viewer_size.y)});
            }
            else if (current_selected_tab == 1)
            {
                //    projection_image_widget.draw({float(right_width - 4), float(viewer_size.y)});
            }
            ImGui::EndGroup();
            ImGui::EndTable();
        }
    }

    std::map<std::string, std::function<std::shared_ptr<ViewerHandler2>()>> viewer_handlers_registry2;
    void registerViewerHandlers2()
    {
        viewer_handlers_registry2.emplace(ImageViewerHandler2::getID(), ImageViewerHandler2::getInstance);
        //   viewer_handlers_registry.emplace(RadiationViewerHandler::getID(), RadiationViewerHandler::getInstance);
        //   viewer_handlers_registry.emplace(ScatterometerViewerHandler::getID(), ScatterometerViewerHandler::getInstance);
    }
};
