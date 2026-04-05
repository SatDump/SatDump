#include "explorer.h"

#include "../../src-interface/main_ui.h"
#include "core/exception.h"
#include "core/plugin.h"
#include "core/resources.h"
#include "core/style.h"
#include "image/image.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "imgui/imgui_filedrop.h"
#include "imgui/imgui_image.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>

#include "dsp/flowgraph/dsp_flowgraph_handler.h"
#include "handlers/dataset/dataset_handler.h"
#include "handlers/dummy_handler.h"
#include "handlers/experimental/decoupled/rec_backend.h"
#include "handlers/experimental/decoupled/rec_frontend.h"
#include "handlers/experimental/decoupled/test/test_http.h"
#include "handlers/experimental/decoupled/test/test_http_client.h"
#include "handlers/product/product_handler.h"
#include "handlers/projection/projection_handler.h"
#include "handlers/vector/shapefile_handler.h"

// TODOREWORK
#include "handlers/experimental/dsp/cyclo_test.h"
#include "handlers/experimental/dsp/fm_test.h"
#include "handlers/experimental/dsp/newrec.h"

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

            // Enable adding handlers to the explorer externally
            eventBus->register_handler<ExplorerSelectHandlerEvent>([this](const ExplorerSelectHandlerEvent &e) { curr_handler = e.h; });

            // Returns all handlers of a specific type if available
            eventBus->register_handler<GetAllOfTypeEvent>(
                [this](const GetAllOfTypeEvent &v)
                {
                    std::function<void(std::shared_ptr<handlers::Handler> &)> recf;
                    recf = [&v, &recf](std::shared_ptr<handlers::Handler> &h)
                    {
                        for (auto &hs : h->getAllSubHandlers())
                        {
                            if (hs->getID() == v.type)
                                v.hs.push_back(hs);
                            recf(hs);
                        }
                    };

                    recf(processing_handler);
                    for (auto &sh : groups_handlers)
                        recf(sh.second);
                    recf(master_handler);
                });

            // Returns all handlers of a specific type if available
            eventBus->register_handler<ExplorerLoadFileEvent>([this](const ExplorerLoadFileEvent &v) { tryOpenFileInExplorer(v.path); });

// Load tip of the day if we can, "randomly"
#ifdef BUILD_IS_DEBUG
            std::string tod_res = "todd.txt";
#else
            std::string tod_res = "tod.txt";
#endif
            if (resources::resourceExists(tod_res))
            {
                std::ifstream message_of_the_day_f(resources::getResourcePath(tod_res));
                std::vector<std::string> lines;
                std::string l;
                while (std::getline(message_of_the_day_f, l))
                    lines.push_back(l);

                if (lego_debug_mode)
                    lines = {
                        {0x57, 0x65, 0x20, 0x73, 0x74, 0x69, 0x6c, 0x6c, 0x20, 0x77, 0x6f, 0x6e, 0x64, 0x65, 0x72, 0x20, 0x77, 0x68, 0x61, 0x74, 0x20, 0x68, 0x61, 0x70, 0x70,
                         0x65, 0x6e, 0x65, 0x64, 0x20, 0x74, 0x6f, 0x20, 0x74, 0x68, 0x65, 0x20, 0x70, 0x6f, 0x6f, 0x72, 0x20, 0x6c, 0x65, 0x67, 0x6f, 0x30, 0x31, 0x20, 0x74,
                         0x6f, 0x20, 0x6c, 0x65, 0x67, 0x6f, 0x31, 0x30, 0x2e, 0x2e, 0x2e, 0x20, 0x41, 0x6e, 0x64, 0x20, 0x6c, 0x65, 0x67, 0x6f, 0x31, 0x32, 0x20, 0x73, 0x65,
                         0x65, 0x6d, 0x73, 0x20, 0x74, 0x6f, 0x20, 0x68, 0x61, 0x76, 0x65, 0x20, 0x76, 0x61, 0x6e, 0x69, 0x73, 0x68, 0x65, 0x64, 0x2c, 0x20, 0x69, 0x74, 0x27,
                         0x73, 0x20, 0x65, 0x78, 0x74, 0x72, 0x65, 0x6d, 0x65, 0x6c, 0x79, 0x20, 0x77, 0x6f, 0x72, 0x72, 0x79, 0x69, 0x6e, 0x67, 0x2e, 0x2e, 0x2e, 0x00}, //
                        {0x4c, 0x65, 0x67, 0x6f, 0x20, 0x70, 0x6c, 0x65, 0x61, 0x73, 0x65, 0x20, 0x66, 0x69, 0x6e, 0x64, 0x20, 0x73, 0x6f, 0x6d, 0x65, 0x74, 0x68, 0x69,
                         0x6e, 0x67, 0x20, 0x6f, 0x74, 0x68, 0x65, 0x72, 0x20, 0x74, 0x68, 0x61, 0x6e, 0x20, 0x6d, 0x6f, 0x73, 0x71, 0x75, 0x69, 0x74, 0x74, 0x6f, 0x65,
                         0x73, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x74, 0x69, 0x6d, 0x65, 0x2e, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65, 0x27, 0x73, 0x20, 0x61, 0x20, 0x73,
                         0x68, 0x6f, 0x72, 0x74, 0x61, 0x67, 0x65, 0x20, 0x6f, 0x66, 0x20, 0x62, 0x75, 0x67, 0x20, 0x72, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x73, 0x21, 0x00}, //
                        {0x44, 0x69, 0x64, 0x20, 0x61, 0x20, 0x63, 0x6f, 0x6d, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x65, 0x20, 0x62, 0x72, 0x65, 0x61,
                         0x6b, 0x20, 0x61, 0x67, 0x61, 0x69, 0x6e, 0x3f, 0x20, 0x50, 0x72, 0x6f, 0x62, 0x61, 0x62, 0x6c, 0x79, 0x2e, 0x00}, //
                        {0x42, 0x44, 0x41, 0x44, 0x61, 0x74, 0x61, 0x45, 0x78, 0x20, 0x70, 0x72, 0x6f, 0x62, 0x61, 0x62, 0x6c, 0x79, 0x20, 0x63, 0x72, 0x61, 0x73, 0x68, 0x65, 0x64,
                         0x20, 0x61, 0x67, 0x61, 0x69, 0x6e, 0x2c, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x73, 0x68, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x2e, 0x00}, //
                        {0x59, 0x6f, 0x75, 0x20, 0x73, 0x74, 0x69, 0x6c, 0x6c, 0x20, 0x6e, 0x65, 0x65, 0x64, 0x20, 0x74, 0x6f, 0x20, 0x61, 0x64, 0x64, 0x20, 0x74, 0x68, 0x65, 0x20, 0x52, 0x45,
                         0x52, 0x20, 0x43, 0x20, 0x28, 0x69, 0x6e, 0x20, 0x66, 0x75, 0x6c, 0x6c, 0x2c, 0x20, 0x70, 0x72, 0x65, 0x2d, 0x4c, 0x69, 0x6e, 0x65, 0x2d, 0x56, 0x20, 0x76, 0x65, 0x72,
                         0x73, 0x69, 0x6f, 0x6e, 0x29, 0x20, 0x6f, 0x6e, 0x20, 0x79, 0x6f, 0x75, 0x72, 0x20, 0x73, 0x69, 0x6d, 0x75, 0x6c, 0x61, 0x74, 0x6f, 0x72, 0x2e, 0x2e, 0x2e, 0x00}, //
                        {0x44, 0X6F, 0X65, 0X73, 0X20, 0X74, 0X68, 0X61, 0X74, 0X20, 0X74, 0X69, 0X6E, 0X79, 0X20, 0X69, 0X6E, 0X73, 0X69, 0X67, 0X6E, 0X69, 0X66, 0X69,
                         0X63, 0X61, 0X6E, 0X74, 0X20, 0X6C, 0X69, 0X74, 0X74, 0X6C, 0X65, 0X20, 0X62, 0X75, 0X74, 0X74, 0X6F, 0X6E, 0X20, 0X62, 0X6F, 0X74, 0X68, 0X65,
                         0X72, 0X20, 0X79, 0X6F, 0X75, 0X20, 0X61, 0X67, 0X61, 0X69, 0X6E, 0X3F, 0X20, 0X54, 0X6F, 0X6F, 0X20, 0X62, 0X61, 0X64, 0X2E, 0x00}, //
                    };

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

                    // If the handler still exist but is not in the tree, we must destroy it
                    // also destroy it in the last_of_type cache
                    if (!handler_present && curr_handler)
                        curr_handler.reset();

                    if ((!master_handler->hasSubhandlers()) && (!groups_handlers.size()))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextDisabled("Handlers will appear here...");
                    }

                    ImGui::EndTable();
                }
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
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Experimental"))
                    { // TODOREWORK?
                        if (ImGui::MenuItem("NewRec TEST"))
                            addHandler(std::make_shared<handlers::NewRecHandler>());
                        if (ImGui::MenuItem("CycloHelper TEST"))
                            addHandler(std::make_shared<handlers::CycloHelperHandler>());
                        if (ImGui::MenuItem("FM Test"))
                            addHandler(std::make_shared<handlers::FMTestHandler>());
                        if (ImGui::MenuItem("TestRemoteRec"))
                            addHandler(std::make_shared<handlers::RecFrontendHandler>(std::make_shared<handlers::TestHttpBackend>(std::make_shared<handlers::RecBackend>())));
                        if (ImGui::MenuItem("TestRemoteClient"))
                            addHandler(std::make_shared<handlers::RecFrontendHandler>(std::make_shared<handlers::TestHttpClientBackend>()));
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
                    ImGui::MenuItem("|", NULL, false, false);
                    curr_handler->drawMenuBar();
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

#if 1
                std::pair<float, float> dims = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};
                float scale = backend::device_scale;

                std::string title = lego_debug_mode ?                                                                                                                //
                                        std::string{0x57, 0x65, 0x6c, 0x63, 0x6f, 0x6d, 0x65, 0x20, 0x74, 0x6f, 0x20, 0x42, 0x75, 0x67, 0x44, 0x75, 0x6d, 0x70, '?'} //
                                                    : "Welcome to SatDump!";

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

                        for (int pp = p + std::min<int>(50, tip_of_the_day.length() - p); pp < tip_of_the_day.length(); pp++)
                        {
                            cutLength = pp;
                            if (tip_of_the_day[pp] == ' ')
                                break;
                        }

                        if (tip_of_the_day.size() > (p + cutLength))
                        {
                            if (tip_of_the_day[p + cutLength] == '.' || tip_of_the_day[p + cutLength] == '?' || tip_of_the_day[p + cutLength] == '!')
                                cutLength++;
                        }

                        if (cutLength == 0)
                            cutLength = tip_of_the_day.length();

                        std::string line = tip_of_the_day.substr(p, cutLength - p);
                        ImVec2 line_size = ImGui::CalcTextSize(line.c_str());
                        last_pos = ((float)dims.second / 2) + ((80 + i * 20) * scale);
                        ImGui::SetCursorPos({((float)dims.first / 2) - (line_size.x / 2), last_pos});
#ifdef BUILD_IS_DEBUG
                        ImGui::TextColored(ImColor(255, 0, 0), "%s", line.c_str());
#else
                        ImGui::TextDisabled("%s", line.c_str());
#endif
                        p += line.size();
                        i++;

                        // Avoid getting stuck in a loop, just in case
                        if (i > 20)
                            break;
                    }

                    {
                        ImVec2 line_size = ImGui::CalcTextSize("Start Processing");
                        ImGui::SetCursorPos({((float)dims.first / 2) - (line_size.x / 2), last_pos + (26 * 1) * scale});
                        if (ImGui::Button("Start Processing"))
                            eventBus->fire_event<ShowProcesingEvent>({}); // TODOREWORK do not bind this directly.
                    }

                    if (lego_debug_mode)
                    {
                        ImVec2 line_size = ImGui::CalcTextSize("Add New Recorder");
                        ImGui::SetCursorPos({((float)dims.first / 2) - (line_size.x / 2), last_pos + (26 * 2) * scale});
                        if (ImGui::Button("Add Buggy Recorder"))
                            addHandler(std::make_shared<handlers::NewRecHandler>());
                        //   addHandler(std::make_shared<handlers::RecFrontendHandler>(std::make_shared<handlers::TestHttpBackend>(std::make_shared<handlers::RecBackend>())));
                    }
                    else
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
            if (explorer_size.x < 0.0f || explorer_size.y < 0.0f)
                return;

            if (show_panel)
            {
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

                    ImGui::BeginChild("ExplorerChildPanel", {left_width, float(explorer_size.y)}, false);
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
            else
            {
                ImGui::BeginGroup();

                if (curr_handler)
                    curr_handler->drawContents({float(explorer_size.x), float(explorer_size.y)});
                else
                    drawContents();

                ImGui::EndGroup();
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
                    else if (std::filesystem::path(path).extension().string() == ".satdump_dsp_flowgraph")
                        loaders.push_back({"DSP Flowgraph Loader", [](std::string path, ExplorerApplication *e)
                                           {
                                               logger->trace("Viewer loading DSP flowgraph " + path);
                                               e->addHandler(std::make_shared<handlers::DSPFlowGraphHandler>(path), true);
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
