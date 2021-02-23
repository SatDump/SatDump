#include "processing.h"
#include "logger.h"
#include "style.h"
#include "gl.h"
#include "logger.h"
#include "module.h"
#include <signal.h>
#include <filesystem>

#include <fstream>
#include "processing.h"
#include <thread>
#include <GLFW/glfw3.h>
#include "imgui/imgui_flags.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include "fft.h"

static void glfw_error_callback(int error, const char *description)
{
    logger->error("Glfw Error " + std::to_string(error) + ": " + description);
}

std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
std::shared_ptr<std::mutex> uiCallListMutex;

std::thread processThread;

std::string downlink_pipeline;
std::string input_level;
std::string input_file;
std::string output_level = "products";
std::string output_file;
std::map<std::string, std::string> parameters;

int pipeline_id = -1;
int input_level_id = -1;

int baseband_type_option = 2;

char samplerate[100];
int frequency_id = 0;
float frequency;
std::string baseband_format;
bool dc_block;

bool livedemod = false;

std::thread demodThread;

int main(int argc, char *argv[])
{
    uiCallList = std::make_shared<std::vector<std::shared_ptr<ProcessingModule>>>();
    uiCallListMutex = std::make_shared<std::mutex>();

// Ignore SIGPIPE
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    initLogger();

    if (argc < 6)
    {
        logger->info("Usage : " + std::string(argv[0]) + " [downlink] [input_level] [input_file] [output_level] [output_file_or_directory]");
        // exit(1);
    }
    else
    {
        downlink_pipeline = argv[1];
        input_level = argv[2];
        input_file = argv[3];
        output_level = argv[4];
        output_file = argv[5];

        if (argc > 6)
        {
            for (int i = 6; i < argc; i++)
            {
                if (i + 1 != argc)
                {
                    if (strcmp(argv[i], "-samplerate") == 0) // This is your parameter name
                    {
                        parameters.emplace("samplerate", argv[i + 1]); // The next value in the array is your value
                        i++;                                           // Move to the next flag
                    }
                    else if (strcmp(argv[i], "-baseband_format") == 0) // This is your parameter name
                    {
                        parameters.emplace("baseband_format", argv[i + 1]); // The next value in the array is your value
                        i++;                                                // Move to the next flag
                    }
                    else if (strcmp(argv[i], "-dc_block") == 0) // This is your parameter name
                    {
                        parameters.emplace("dc_block", argv[i + 1]); // The next value in the array is your value
                        i++;                                         // Move to the next flag
                    }
                }
            }
        }

        processing = true;
    }

    logger->info("   _____       __  ____                      ");
    logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
    logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
    logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
    logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
    logger->info("                                    /_/      ");
    logger->info("Starting SatDump v1.0");
    logger->info("");

    registerModules();

    loadPipelines("../pipelines.json");

    logger->debug("Registered pipelines :");
    for (Pipeline &pipeline : pipelines)
        logger->debug(" - " + pipeline.name);

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        logger->critical("Could not init GLFW");
        exit(1);
    }

    // GL STUFF

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1000, 600, "SatDump", NULL, NULL);
    if (window == NULL)
    {
        logger->critical("Could not init GLFW Window");
        exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    if (glewInit() != GLEW_OK)
    {
        logger->critical("Failed to initialize OpenGL loader!");
        exit(1);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.IniFilename = NULL;

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    style::setDarkStyle("..");

    if (processing)
        processThread = std::thread(&process, downlink_pipeline, input_level, input_file, output_level, output_file, parameters);

    std::shared_ptr<SDRSource> airspySource;
    std::shared_ptr<ProcessingModule> demodModule;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int wwidth, wheight;
        glfwGetWindowSize(window, &wwidth, &wheight);

        // User rendering
        {
            if (processing)
            {
                uiCallListMutex->lock();
                for (std::shared_ptr<ProcessingModule> module : *uiCallList)
                {
                    ImGui::SetNextWindowPos({0, 0});
                    ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
                    module->drawUI();
                }
                uiCallListMutex->unlock();

                //ImGui::SetNextWindowPos({0, wheight - 100});
                //ImGui::SetNextWindowSize({wwidth, 100});
                //ImGui::Begin("Status");

                //ImGui::End();
            }
            else if (livedemod)
            {
                ImGui::SetNextWindowPos({0, 0});
                ImGui::SetNextWindowSize({(float)wwidth, (float)wheight - 265});
                airspySource->drawUI();
                ImGui::SetNextWindowPos({0, (float)wheight - 265});
                ImGui::SetNextWindowSize({(float)wwidth, (float)265});
                demodModule->drawUI();
            }
            else
            {
                ImGui::SetNextWindowPos({0, 0});
                ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
                ImGui::Begin("Main Window", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoTitleBar);

                if (ImGui::BeginTabBar("Main TabBar", ImGuiTabBarFlags_None))
                {
                    if (ImGui::BeginTabItem("Offline processing"))
                    {
                        ImGui::BeginGroup();
                        {
                            std::string names;

                            for (int i = 0; i < pipelines.size(); i++)
                            {
                                names += pipelines[i].readable_name + '\0';
                            }

                            ImGui::Text("Pipeline      : ");
                            ImGui::SameLine();

                            if (ImGui::Combo("##pipeline", &pipeline_id, names.c_str()))
                            {
                                downlink_pipeline = pipelines[pipeline_id].name;
                                std::memcpy(samplerate, std::to_string(pipelines[pipeline_id].default_samplerate).c_str(), std::to_string(pipelines[pipeline_id].default_samplerate).length());

                                if (pipelines[pipeline_id].default_baseband_type == "f32")
                                {
                                    baseband_type_option = 0;
                                }
                                else if (pipelines[pipeline_id].default_baseband_type == "i8")
                                {
                                    baseband_type_option = 2;
                                }
                                else if (pipelines[pipeline_id].default_baseband_type == "i16")
                                {
                                    baseband_type_option = 1;
                                }
                                else if (pipelines[pipeline_id].default_baseband_type == "w8")
                                {
                                    baseband_type_option = 4;
                                }
                                baseband_format = pipelines[pipeline_id].default_baseband_type;
                            }
                            // ImGui::EndCombo();
                        }
                        ImGui::EndGroup();

                        ImGui::BeginGroup();
                        {
                            ImGui::Text("Input Level : ");
                            ImGui::SameLine();

                            // logger->info(names);

                            std::string names; // = "baseband\0";

                            if (pipeline_id != -1)
                                for (int i = 0; i < pipelines[pipeline_id].steps.size(); i++)
                                {
                                    names += pipelines[pipeline_id].steps[i].level_name + '\0';
                                }

                            if (ImGui::Combo("##input_level", &input_level_id, names.c_str()))
                            {
                                input_level = pipelines[pipeline_id].steps[input_level_id].level_name;
                            }
                        }
                        ImGui::EndGroup();

                        ImGui::BeginGroup();
                        {
                            ImGui::Text("Input File : ");
                            ImGui::SameLine();

                            if (ImGui::Button("Select Input"))
                            {
                                logger->debug("Opening file dialog");
                                /* auto result = pfd::open_file("Open input file", ".", {".*"}, false);
                                while (result.ready(1000))
                                {
                                }

                                if (result.result().size() > 0)
                                    input_file = result.result()[0];*/
                                char const *result = tinyfd_openFileDialog("Open input file", // NULL or ""
                                                                           NULL,              // NULL or ""
                                                                           0,                 // 0 (2 in the following example)
                                                                           NULL,              // NULL or char const * lFilterPatterns[2]={"*.png","*.jpg"};
                                                                           "",                // NULL or "image files"
                                                                           0);                // 0
                                                                                              // in case of multiple files, the separator is |
                                                                                              // returns NULL on cancel
                                if (result != NULL)
                                    input_file = result;
                                logger->debug("Dir " + input_file);
                            }

                            ImGui::SameLine();
                            ImGui::Text(input_file.c_str());
                        }
                        ImGui::EndGroup();

                        ImGui::BeginGroup();
                        {
                            ImGui::Text("Output dir : ");
                            ImGui::SameLine();

                            if (ImGui::Button("Select Output"))
                            {
                                logger->debug("Opening file dialog");
                                /*auto result = pfd::select_folder("Open output directory", ".");
                                while (result.ready(1000))
                                {
                                }

                                if (result.result().size() > 0)
                                    output_file = result.result();*/
                                char const *result = tinyfd_selectFolderDialog(
                                    "Open output directory", // NULL or ""
                                    NULL);                   // NULL or ""
                                // returns NULL on cancel
                                if (result != NULL)
                                    output_file = result;
                                logger->debug("Dir " + output_file);
                            }

                            ImGui::SameLine();
                            ImGui::Text(output_file.c_str());
                        }
                        ImGui::EndGroup();

                        ImGui::BeginGroup();
                        {
                            ImGui::Text("Parameters : ");

                            if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_Borders))
                            {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("Samplerate (Hz / SPS)");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::InputText("  ", samplerate, 100);

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("Baseband Type");
                                ImGui::TableSetColumnIndex(1);
                                if (ImGui::Combo("", &baseband_type_option, "f32\0i16\0i8\0w8\0"))
                                {
                                    switch (baseband_type_option)
                                    {
                                    case 0:
                                        baseband_format = "f32";
                                        break;
                                    case 1:
                                        baseband_format = "i16";
                                        break;
                                    case 2:
                                        baseband_format = "i8";
                                        break;
                                    case 3:
                                        baseband_format = "w8";
                                        break;
                                    }
                                }

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("DC Block");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Checkbox("DC Block", &dc_block);

                                ImGui::EndTable();
                            }
                        }
                        ImGui::EndGroup();

                        ImGui::BeginGroup();
                        {
                            if (ImGui::Button("Start"))
                            {
                                logger->debug("Starting...");

                                parameters.emplace("samplerate", std::string(samplerate));

                                parameters.emplace("baseband_format", baseband_format);

                                parameters.emplace("dc_block", dc_block ? "1" : "0");

                                processThread = std::thread(&process, downlink_pipeline, input_level, input_file, output_level, output_file, parameters);
                                //showStartup = false;
                            }
                        }
                        ImGui::EndGroup();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Live processing"))
                    {
                        ImGui::BeginGroup();
                        {
                            std::string names;

                            std::map<int, std::string> id_table;

                            for (int i = 0, y = 0; i < pipelines.size(); i++)
                            {
                                if (pipelines[i].live)
                                {
                                    names += pipelines[i].readable_name + '\0';
                                    id_table.insert({y++, pipelines[i].name});
                                }
                            }

                            ImGui::Text("Pipeline      : ");
                            ImGui::SameLine();

                            if (ImGui::Combo("##pipeline", &pipeline_id, names.c_str()))
                            {
                                downlink_pipeline = id_table[pipeline_id];
                                std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                                                  pipelines.end(),
                                                                                  [](const Pipeline &e) {
                                                                                      return e.name == downlink_pipeline;
                                                                                  });
                                std::memcpy(samplerate, std::to_string(it->default_samplerate).c_str(), std::to_string(it->default_samplerate).length());
                                frequency = it->frequencies[frequency_id];
                            }
                            // ImGui::EndCombo();
                        }
                        ImGui::EndGroup();

                        ImGui::BeginGroup();
                        {
                            ImGui::Text("Output dir : ");
                            ImGui::SameLine();

                            if (ImGui::Button("Select Output"))
                            {
                                logger->debug("Opening file dialog");
                                /*auto result = pfd::select_folder("Open output directory", ".");
                                while (result.ready(1000))
                                {
                                }

                                if (result.result().size() > 0)
                                    output_file = result.result();*/
                                logger->debug("Dir " + output_file);
                            }

                            ImGui::SameLine();
                            ImGui::Text(output_file.c_str());
                        }
                        ImGui::EndGroup();

                        ImGui::Separator();

                        ImGui::BeginGroup();
                        {
                            ImGui::Text("Frequency : ");
                            ImGui::SameLine();

                            // logger->info(names);

                            std::string names; // = "baseband\0";

                            std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                                              pipelines.end(),
                                                                              [](const Pipeline &e) {
                                                                                  return e.name == downlink_pipeline;
                                                                              });

                            if (it != pipelines.end())
                                for (int i = 0; i < it->frequencies.size(); i++)
                                {
                                    names += std::to_string(it->frequencies[i]) + " Mhz" + '\0';
                                }

                            if (ImGui::Combo("##frequency", &frequency_id, names.c_str()))
                            {
                                frequency = it->frequencies[frequency_id];
                            }

                            ImGui::Checkbox("DC Block", &dc_block);

                            ImGui::Text("Samplerate (Hz / SPS)");
                            ImGui::SameLine();
                            ImGui::InputText("##samplerate", samplerate, 100);

                            if (ImGui::Button("Start"))
                            {
                                logger->debug("Starting livedemod...");

                                if (!std::filesystem::exists(output_file))
                                    std::filesystem::create_directory(output_file);

                                logger->info(downlink_pipeline);

                                std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                                                  pipelines.end(),
                                                                                  [](const Pipeline &e) {
                                                                                      return e.name == downlink_pipeline;
                                                                                  });

                                if (it != pipelines.end())
                                {

                                    parameters.emplace("samplerate", std::string(samplerate));

                                    parameters.emplace("baseband_format", "f32");

                                    parameters.emplace("dc_block", dc_block ? "1" : "0");

                                    std::map<std::string, std::string> final_parameters = it->steps[1].modules[0].parameters;
                                    for (const std::pair<std::string, std::string> &param : parameters)
                                        if (final_parameters.count(param.first) > 0)
                                            final_parameters[param.first] = param.second;
                                        else
                                            final_parameters.emplace(param.first, param.second);

                                    logger->debug("Parameters :");
                                    for (const std::pair<std::string, std::string> &param : final_parameters)
                                        logger->debug("   - " + param.first + " : " + param.second);

                                    demodModule = modules_registry[it->steps[1]
                                                                       .modules[0]
                                                                       .module_name]("", output_file + "/" + it->name, final_parameters);

                                    demodModule->setInputType(DATA_STREAM);
                                    demodModule->setOutputType(DATA_FILE);
                                    demodModule->input_fifo = std::make_shared<satdump::Pipe>();
                                    demodModule->input_active = true;

                                    airspySource = std::make_shared<SDRSource>((float)frequency * 1e6, std::stoi(samplerate), demodModule->input_fifo);

                                    demodThread = std::thread([&]() {
                                        logger->info("Start processing...");
                                        demodModule->process();
                                    });

                                    airspySource->stopFuction = [&]() {
                                        logger->info("Stop live");
                                        demodModule->input_active = false;
                                        demodModule->input_fifo->~Pipe();
                                        logger->info("Pipe OK");
                                        if (demodThread.joinable())
                                            demodThread.join();
                                        logger->info("Stopped");
                                        demodThread.~thread();
                                        livedemod = false;
                                        logger->info("Done");
                                    };

                                    airspySource->startSDR();

                                    livedemod = true;
                                }
                                else
                                {
                                    logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
                                }
                            }
                        }
                        ImGui::EndGroup();

                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Credits"))
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
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();

                ImGui::End();
            }
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
        //glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    if (processThread.joinable())
        processThread.join();
}