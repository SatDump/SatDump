#include "logger.h"
#include "style.h"
#include "gl.h"
#include "logger.h"
#include <signal.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui_flags.h"
#include "credits.h"
#include "global.h"
#include "init.h"
#include "processing.h"
#include "offline.h"
#include "live.h"
#include "live_run.h"

static void glfw_error_callback(int error, const char *description)
{
    logger->error("Glfw Error " + std::to_string(error) + ": " + description);
}

int main(int argc, char *argv[])
{
    std::fill(error_message, &error_message[0], 0);

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

    initSatdump();

#ifdef BUILD_LIVE
    initLive();
#endif

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
    if (!ImGui_ImplOpenGL3_Init("#version 150"))
        ImGui_ImplOpenGL3_Init("#version 120"); // If 1.5 doesn't work go back to 1.2

    style::setDarkStyle(".");

    if (processing)
        processThreadPool.push([&](int) { process(downlink_pipeline, input_level, input_file, output_level, output_file, parameters); });

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
                    module->drawUI(false);
                }
                uiCallListMutex->unlock();
            }
#ifdef BUILD_LIVE
            else if (live_processing)
            {
                renderLive();
            }
#endif
            else
            {
                ImGui::SetNextWindowPos({0, 0});
                ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
                ImGui::Begin("Main Window", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoTitleBar);

                if (ImGui::BeginTabBar("Main TabBar", ImGuiTabBarFlags_None))
                {
                    if (ImGui::BeginTabItem("Offline processing"))
                    {
                        renderOfflineProcessing();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Live processing"))
                    {
#ifdef BUILD_LIVE
                        renderLiveProcessing();
#else
                        ImGui::Text("Live support is currently being rewritten, and does not work on Windows yet. If you really need it, please stick to an older version.");
#endif
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Credits"))
                    {
                        renderCredits(wwidth, wheight);
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

    processThreadPool.stop();

    for (int i = 0; i < processThreadPool.size(); i++)
    {
        if (processThreadPool.get_thread(i).joinable())
            processThreadPool.get_thread(i).join();
    }
}