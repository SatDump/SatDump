#include "logger.h"
#include "style.h"
#include "gl.h"
#include "logger.h"
#include <signal.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui_flags.h"
#include "global.h"
#include "init.h"
#include "processing.h"
#include <filesystem>
#include "settings.h"
#include "settingsui.h"
#include "main_ui.h"
#include "live/live.h"
#include "satdump_vars.h"
#include "tle.h"
#include "recorder/recorder.h"

extern bool recorder_running;

static void glfw_error_callback(int error, const char *description)
{
    logger->error("Glfw Error " + std::to_string(error) + ": " + description);
}

void bindImageTextureFunctions();
void bindFileDialogsFunctions();

int main(int argc, char *argv[])
{
    bindImageTextureFunctions();
    bindFileDialogsFunctions();
    //std::fill(error_message, &error_message[0], 0);

    uiCallList = std::make_shared<std::vector<std::shared_ptr<ProcessingModule>>>();
    uiCallListMutex = std::make_shared<std::mutex>();

// Ignore SIGPIPE
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    initLogger();

    std::string downlink_pipeline = "";
    std::string input_level = "";
    std::string input_file = "";
    std::string output_level = "products";
    std::string output_file = "";
    nlohmann::json parameters;

    if (argc < 6 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        logger->info("Usage : " + std::string(argv[0]) + " [downlink] [input_level] [input_file] [output_level] [output_file_or_directory] [additional options as required]");
        logger->info("Extra options : -samplerate [baseband_samplerate] -baseband_format [f32/i16/i8/w8] -dc_block -iq_swap");
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
                        parameters["samplerate"] = std::stol(argv[i + 1]); // The next value in the array is your value
                        i++;                                               // Move to the next flag
                    }
                    else if (strcmp(argv[i], "-baseband_format") == 0) // This is your parameter name
                    {
                        parameters["baseband_format"] = argv[i + 1]; // The next value in the array is your value
                        i++;                                         // Move to the next flag
                    }
                    else if (strcmp(argv[i], "-dc_block") == 0) // This is your parameter name
                    {
                        parameters["dc_block"] = (bool)std::stoi(argv[i + 1]); // The next value in the array is your value
                        i++;                                                   // Move to the next flag
                    }
                    else if (strcmp(argv[i], "-iq_swap") == 0) // This is your parameter name
                    {
                        parameters["iq_swap"] = (bool)std::stoi(argv[i + 1]); // The next value in the array is your value
                        i++;                                                  // Move to the next flag
                    }
                }
            }
        }

        satdumpUiStatus = OFFLINE_PROCESSING;
    }

    initSatdump();

#ifdef _WIN32
    loadSettings("settings.json");
#else
    if (std::filesystem::exists("settings.json"))
    {
        loadSettings("settings.json");
    }
    else
    {
        std::string cfg_path = std::string(getenv("HOME")) + "/.config/satdump";

        if (!std::filesystem::exists(cfg_path))
        {
            logger->debug("Creating directory " + cfg_path);
            std::filesystem::create_directories(cfg_path);
        }

        loadSettings(cfg_path + "/settings.json");
    }
#endif

    parseSettingsOrDefaults();

    // If we are asked to update TLEs on boot, do so
    if (update_tles_on_startup)
        tle::updateTLEsMT();

    // Init UI
    initMainUI();

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
    GLFWwindow *window = glfwCreateWindow(1000, 600, std::string("SatDump v" + (std::string)SATDUMP_VERSION).c_str(), NULL, NULL);
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
    if (!ImGui_ImplOpenGL3_Init("#version 150"))
        ImGui_ImplOpenGL3_Init("#version 120"); // If 1.5 doesn't work go back to 1.2

    if (std::filesystem::exists("Roboto-Medium.ttf"))
    {
        if (use_light_theme)
            style::setLightStyle(".", manual_dpi_scaling);
        else
            style::setDarkStyle(".", manual_dpi_scaling);
    }
    else
    {
        if (use_light_theme)
            style::setLightStyle((std::string)RESOURCES_PATH);
        else
            style::setDarkStyle((std::string)RESOURCES_PATH);
    }

    if (satdumpUiStatus == OFFLINE_PROCESSING)
        processThreadPool.push([&](int)
                               { processing::process(downlink_pipeline, input_level, input_file, output_level, output_file, parameters); });

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
        renderMainUI(wwidth, wheight);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        if (use_light_theme)
            glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        else
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

    tle::stopTLECleanMT(); // Let the TLE update thread shutdown cleanly

    logger->info("UI Exit");

    // If we're doing live processing, we want this to kill all threads quickly. Hence don't call destructors
    if (satdumpUiStatus == OFFLINE_PROCESSING)
#ifdef __APPLE__
        exit(0);
#else
        quick_exit(0);
#endif

#ifdef BUILD_LIVE
    if (satdumpUiStatus == BASEBAND_RECORDER)
    {
        recorder::exitRecorder();
// Same story as offline processing, except we finish recording first
#ifdef __APPLE__
        exit(0);
#else
        quick_exit(0);
#endif
    }
#endif

    processThreadPool.stop();

    for (int i = 0; i < processThreadPool.size(); i++)
    {
        if (processThreadPool.get_thread(i).joinable())
            processThreadPool.get_thread(i).join();
    }

    logger->info("Exiting!");
}
