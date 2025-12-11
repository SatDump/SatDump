#include "explorer.h"

#include "common/utils.h"

#include "core/exception.h"
#include "core/plugin.h"

#include "core/style.h"
#include "dsp/cyclo_test.h"
#include "dsp/flowgraph/dsp_flowgraph_handler.h"
#include "handlers/dataset/dataset_handler.h"
#include "handlers/dummy_handler.h"
#include "handlers/product/image_product_handler.h" // TODOREWORK CLEAN
#include "handlers/projection/projection_handler.h"
#include "handlers/vector/shapefile_handler.h"
// TODOREWORK
#include "core/resources.h"
#include "image/image.h"
#include "imgui/imgui.h"

// TODOREWORK
#include "dsp/newrec.h"
#include "dsp/waterfall_test.h"

#include "image/io.h"

#include "../../src-interface/main_ui.h"
#include "imgui/imgui_filedrop.h"
#include "imgui/imgui_image.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

namespace satdump
{
    namespace explorer
    {
        ExplorerApplication::ExplorerApplication()
        {
            master_handler = std::make_shared<handlers::DummyHandler>("MasterHandlerExplorer");

            // Add processing handler "spot"
            processing_handler = std::make_shared<handlers::DummyHandler>("ProcessingHandlerExplorer");
            processing_handler->setCanBeDraggedTo(false);

            // Enable dropping files onto the explorer.
            eventBus->register_handler<imgui_utils::FileDropEvent>(
                [this](const imgui_utils::FileDropEvent &v)
                {
                    for (auto &f : v.files)
                        tryOpenFileInExplorer(f);
                });

            // Enable adding handlers to the explorer externally
            eventBus->register_handler<ExplorerAddHandlerEvent>([this](const ExplorerAddHandlerEvent &e) { addHandler(e.h, e.open, e.is_processing); });

            // Returns the last selected handler of a specific type if available
            eventBus->register_handler<GetLastSelectedOfTypeEvent>(
                [this](const GetLastSelectedOfTypeEvent &v)
                {
                    if (last_selected_handler.count(v.type))
                        v.h = last_selected_handler[v.type];
                    else
                        v.h = nullptr;
                });

            // Load if the day if we can, "randomly"
            if (resources::resourceExists("tod.txt"))
            {
                std::ifstream message_of_the_day_f(resources::getResourcePath("tod.txt"));
                std::vector<std::string> lines;
                std::string l;
                while (std::getline(message_of_the_day_f, l))
                    lines.push_back(l);
                int this_line = time(0) % lines.size();
                tip_of_the_day = lines[this_line];
            }
        }

        ExplorerApplication::~ExplorerApplication() {}

        void ExplorerApplication::addHandler(std::shared_ptr<handlers::Handler> h, bool open, bool is_processing)
        {
            if (is_processing)
                processing_handler->addSubHandler(h);
            else
            {
                // Try adding to a group if possible
                bool added = false;
                for (auto &v : group_definitions)
                {
                    for (auto &v2 : v.second)
                    {
                        if (v2 == h->getID())
                        {
                            if (groups_handlers.count(v.first) == 0)
                                groups_handlers.emplace(v.first, std::make_shared<handlers::DummyHandler>(v.first + "HandlerExplorer"));
                            groups_handlers[v.first]->addSubHandler(h);
                            added = true;
                        }
                    }
                }

                if (!added)
                    master_handler->addSubHandler(h);
            }
            if (open)
                curr_handler = h;
        }

        void ExplorerApplication::drawPanel()
        {
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);

            bool handler_present = false;

            if (ImGui::CollapsingHeader("Root", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto prev_curr = curr_handler;

                if (ImGui::BeginTable("##masterhandlertable", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("##masterhandlertable_col", ImGuiTableColumnFlags_None);
                    ImGui::TableNextColumn();

                    bool rendering_separators = false;

                    if (processing_handler->hasSubhandlers())
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SeparatorText("Processing");

                        handler_present |= processing_handler->drawTreeMenu(curr_handler);
                        for (auto &h : processing_handler->getAllSubHandlers())
                            if (h->getName() == "PROCESSING_DONE") // TODOREWORK MASSIVE HACK
                                processing_handler->delSubHandler(h);

                        rendering_separators = true;
                    }

                    for (auto &v : groups_handlers)
                    {
                        if (v.second->hasSubhandlers())
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::SeparatorText(v.first.c_str());

                            handler_present |= v.second->drawTreeMenu(curr_handler);
                            rendering_separators = true;
                        }
                    }

                    if (rendering_separators)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SeparatorText("Others");
                    }

                    handler_present |= master_handler->drawTreeMenu(curr_handler);

                    if ((!master_handler->hasSubhandlers()) && (!groups_handlers.size()))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextDisabled("Handlers will appear here...");
                    }

                    ImGui::EndTable();
                }

                if (prev_curr != curr_handler)
                    last_selected_handler.insert_or_assign(curr_handler->getID(), curr_handler);
            }

            if (!handler_present)
                curr_handler.reset();

            if (curr_handler)
                curr_handler->drawMenu();
        }

        void ExplorerApplication::drawMenuBar()
        {
            // Handle File Stuff TODOREWORK?
            {
                if (file_open_dialog.update())
                {
                    std::string prod_path = file_open_dialog.getPath();
                    if (prod_path.size() > 0)
                        tryOpenFileInExplorer(prod_path);
                    else
                        logger->trace("No file selected");
                }
            }

            if (ImGui::BeginMenuBar())
            {
                // Main "Open" menu, for files, other handlers, etc
                if (ImGui::BeginMenu("File"))
                {
                    file_open_dialog.render("Open File", "Open File", "", {{"All Files", "*"}});

                    if (ImGui::BeginMenu("Quick Open"))
                    {
                        ImGui::InputText("##quickvieweropen", &quickOpenString);
                        bool go = ImGui::IsItemDeactivatedAfterEdit();
                        ImGui::SameLine();
                        if (ImGui::Button("Go##loadfromhttpgo") || go)
                        {
                            tryOpenFileInExplorer(quickOpenString);
                            quickOpenString.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Paste & Load"))
                        {
                            // Returns nullptr if clipboard didn't have text or failed for some reason
                            if (ImGui::GetClipboardText() != nullptr)
                            {
                                tryOpenFileInExplorer(std::string(ImGui::GetClipboardText()));
                            }
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Add"))
                {
                    if (ImGui::MenuItem("Projection"))
                        addHandler(std::make_shared<handlers::ProjectionHandler>());

                    if (ImGui::BeginMenu("Tools"))
                    { // TODOREWORK?
                        if (ImGui::MenuItem("DSP Flowgraph"))
                            addHandler(std::make_shared<handlers::DSPFlowGraphHandler>());
                        if (ImGui::MenuItem("Waterfall TEST"))
                            addHandler(std::make_shared<handlers::WaterfallTestHandler>());
                        if (ImGui::MenuItem("NewRec TEST"))
                            addHandler(std::make_shared<handlers::NewRecHandler>());
                        if (ImGui::MenuItem("CycloHelper TEST"))
                            addHandler(std::make_shared<handlers::CycloHelperHandler>());
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Handler", bool(curr_handler)))
                {
                    if (curr_handler && ImGui::BeginMenu("Config"))
                    {
                        if (ImGui::MenuItem("JSON To Clipboard"))
                        {
                            ImGui::SetClipboardText(curr_handler->getConfig().dump(4).c_str());
                            logger->warn("Copied to clipboard!");
                        }
                        if (ImGui::MenuItem("JSON From Clipboard"))
                        {
                            curr_handler->setConfig(nlohmann::json::parse(ImGui::GetClipboardText()));
                            logger->warn("Copied from clipboard!");
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }

                // TODOREWORK
                if (curr_handler)
                {
                    if (ImGui::BeginMenu("Current"))
                    {
                        curr_handler->drawMenuBar();
                        ImGui::EndMenu();
                    }
                }

                eventBus->fire_event<RenderLoadMenuElementsEvent>({curr_handler, master_handler});

                ImGui::EndMenuBar();
            }
        }

        void ExplorerApplication::drawContents()
        {
            if (ImGui::BeginChild("WelcomeChild"))
            {
                if (satdump_logo_texture == 0)
                {
                    image::Image img;
                    image::load_png(img, resources::getResourcePath("icon.png"));
                    satdump_logo_texture = makeImageTexture();
                    uint32_t *px = new uint32_t[img.width() * img.height()];
                    image::image_to_rgba(img, px);
                    updateImageTexture(satdump_logo_texture, (uint32_t *)px, img.width(), img.height());
                    delete[] px;
                }

#if 0
                ImGui::Image((void *)satdump_logo_texture, ImVec2(50 * ui_scale, 50 * ui_scale));
                ImGui::SameLine();
                ImGui::PushFont(style::bigFont);
                ImGui::TextUnformatted(" Welcome to SatDump!");
                ImGui::PopFont();

                if (ImGui::BeginTable("##dscovrinstrumentstable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, {-1, ImGui::GetWindowHeight()}))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Shortcuts");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Tip of the day");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Something else!?");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Yes, why not?");

                    ImGui::EndTable();
                }
#else
                std::pair<float, float> dims = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};
                float scale = backend::device_scale;

                std::string title = "Welcome to SatDump!";

                {
                    ImGui::PushFont(style::bigFont);
                    ImVec2 title_size = ImGui::CalcTextSize(title.c_str());
                    ImGui::SetCursorPos({((float)dims.first / 2) - (75 * scale), ((float)dims.second / 2.f) - title_size.y - (90 * scale)});
                    ImGui::Image((void *)satdump_logo_texture, ImVec2(150 * scale, 150 * scale));
                    ImGui::SetCursorPos({((float)dims.first / 2) - (title_size.x / 2), ((float)dims.second / 2.0f) - title_size.y + (75 * scale)});
                    ImGui::TextUnformatted(title.c_str());
                    ImGui::PopFont();

                    float last_pos = 0;
                    for (int p = 0, i = 0; p < tip_of_the_day.length();)
                    {
                        int cutLength = 0;
                        for (int pp = p + 50; pp < tip_of_the_day.length(); pp++)
                        {
                            cutLength = pp;
                            if (tip_of_the_day[pp] == ' ')
                                break;
                        }
                        std::string line = tip_of_the_day.substr(p, cutLength - p);
                        ImVec2 line_size = ImGui::CalcTextSize(line.c_str());
                        last_pos = ((float)dims.second / 2) + ((80 + i * 20) * scale);
                        ImGui::SetCursorPos({((float)dims.first / 2) - (line_size.x / 2), last_pos});
                        ImGui::TextDisabled("%s", line.c_str());
                        p += line.size();
                        i++;
                    }

                    {
                        ImVec2 line_size = ImGui::CalcTextSize("Start Processing");
                        ImGui::SetCursorPos({((float)dims.first / 2) - (line_size.x / 2), last_pos + (26 * 1) * scale});
                        if (ImGui::Button("Start Processing"))
                            eventBus->fire_event<ShowProcesingEvent>({}); // TODOREWORK do not bind this directly.
                    }

                    {
                        ImVec2 line_size = ImGui::CalcTextSize("Add Recorder");
                        ImGui::SetCursorPos({((float)dims.first / 2) - (line_size.x / 2), last_pos + (26 * 2) * scale});
                        if (ImGui::Button("Add Recorder"))
                            eventBus->fire_event<AddRecorderEvent>({}); // TODOREWORK do not bind this directly.
                    }
                }
#endif

                ImGui::EndChild();
            }
        }

        void ExplorerApplication::draw()
        {
            drawMenuBar();

            ImVec2 explorer_size = ImGui::GetContentRegionAvail();

            if (ImGui::BeginTable("##explorer_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_None, explorer_size.x * panel_ratio);
                ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_None, explorer_size.x * (1.0f - panel_ratio));
                ImGui::TableNextColumn();

                float left_width = ImGui::GetColumnWidth(0);
                float right_width = explorer_size.x - left_width;
                if (left_width != last_width && last_width != -1)
                    panel_ratio = left_width / explorer_size.x;
                last_width = left_width;

                ImGui::BeginChild("ExplorerChildPanel", {left_width, float(explorer_size.y - 10)}, false);
                drawPanel();
                ImGui::EndChild();

                ImGui::TableNextColumn();
                ImGui::BeginGroup();

                if (curr_handler)
                    curr_handler->drawContents({float(right_width - 9 * ui_scale), float(explorer_size.y)});
                else
                    drawContents();

                ImGui::EndGroup();
                ImGui::EndTable();
            }
        }

        void ExplorerApplication::tryOpenFileInExplorer(std::string path)
        {
            auto fun = [this, path]()
            {
                eventBus->fire_event<SetIsProcessingEvent>({});

                try
                {
                    // Get available loaders
                    std::vector<std::pair<std::string, std::function<void(std::string, ExplorerApplication *)>>> loaders;

                    // Default stuff
                    if (std::filesystem::path(path).extension().string() == ".cbor")
                        loaders.push_back({"Product Loader", [](std::string path, ExplorerApplication *e)
                                           {
                                               logger->trace("Explorer loading product " + path);

                                               try
                                               {
                                                   auto prod_h = handlers::getProductHandlerForProduct(products::loadProduct(path));
                                                   e->addHandler(prod_h);
                                               }
                                               catch (std::exception &e)
                                               {
                                                   logger->error("Error loading product! Maybe not a valid product? (Reminder : Pre-2.0.0 products are NOT compatible with 2.0.0) Details : %s",
                                                                 e.what());
                                               }
                                           }});
                    else if (std::filesystem::path(path).extension().string() == ".json")
                        loaders.push_back(
                            {"Dataset Loader", [](std::string path, ExplorerApplication *e)
                             {
                                 logger->trace("Viewer loading dataset " + path);

                                 try
                                 {
                                     products::DataSet dataset;
                                     dataset.load(path);
                                     std::shared_ptr<handlers::DatasetHandler> dat_h = std::make_shared<handlers::DatasetHandler>(std::filesystem::path(path).parent_path().string(), dataset);
                                     e->addHandler(dat_h);
                                 }
                                 catch (std::exception &e)
                                 {
                                     logger->error("Error loading dataset! Maybe not a valid dataset? (Reminder : Pre-2.0.0 datasets are NOT compatible with 2.0.0) Details : %s", e.what());
                                 }
                             }});
                    else if (std::filesystem::path(path).extension().string() == ".shp")
                        loaders.push_back({"Shapefile Loader", [](std::string path, ExplorerApplication *e)
                                           {
                                               logger->trace("Viewer loading shapefile " + path);
                                               e->addHandler(std::make_shared<handlers::ShapefileHandler>(path));
                                           }});

                    // Plugin loaders
                    eventBus->fire_event<ExplorerRequestFileLoad>({path, loaders});

#if 1 // TODOREWORK ADD MENU
                    if (loaders.size() == 0)
                        loaders.push_back({"Image Loader", [](std::string path, ExplorerApplication *e)
                                           {
                                               logger->trace("Viewer loading image " + path);

                                               image::Image img;
                                               image::load_img(img, path);
                                               if (img.size() > 0)
                                                   e->addHandler(std::make_shared<handlers::ImageHandler>(img, std::filesystem::path(path).stem().string()));
                                               else
                                                   logger->error("Could not open this file as image!");
                                           }});
#endif

                    // Load it
                    if (loaders.size() == 1)
                    {
                        loaders[0].second(path, this);
                    }
                    else if (loaders.size() > 1)
                    {
                        logger->error("More than one loader available! TODO SELECTION MENU!");
                        loaders[loaders.size() - 1].second(path, this);
                    }
                    else
                    {
                        throw satdump_exception("No loader found for file : " + path + "!");
                    }
                }
                catch (std::exception &e)
                {
                    logger->error("Error opening file! => %s", e.what());
                }

                eventBus->fire_event<SetIsDoneProcessingEvent>({});
            };
            file_open_queue.push(fun);
        }

        void ExplorerApplication::tryOpenSomethingInExplorer(std::function<void(ExplorerApplication *)> f)
        {
            auto fun = [f, this]()
            {
                eventBus->fire_event<SetIsProcessingEvent>({});

                try
                {
                    f(this);
                }
                catch (std::exception &e)
                {
                    logger->error("Error opening file! => %s", e.what());
                }

                eventBus->fire_event<SetIsDoneProcessingEvent>({});
            };
            file_open_queue.push(fun);
        }
    } // namespace explorer
}; // namespace satdump
