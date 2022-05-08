#include "logger.h"
#include "core/style.h"
#include "gl.h"
#include "logger.h"
#include <signal.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui_flags.h"
#include "init.h"
#include "processing.h"
#include <filesystem>
#include "main_ui.h"
#include "satdump_vars.h"
//#include "tle.h"
#include "common/cli_utils.h"

extern bool recorder_running;

static void glfw_error_callback(int error, const char *description)
{
    logger->error("Glfw Error " + std::to_string(error) + ": " + description);
}

void bindImageTextureFunctions();
// void bindFileDialogsFunctions();

int main(int argc, char *argv[])
{
    bindImageTextureFunctions();
    initLogger();

    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " [downlink] [input_level] [input_file] [output_file_or_directory] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in modules can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [f32/i16/i8/w8] --dc_block --iq_swap");
    }
    else
        satdump::processing::is_processing = true;

    std::string downlink_pipeline = satdump::processing::is_processing ? argv[1] : "";
    std::string input_level = satdump::processing::is_processing ? argv[2] : "";
    std::string input_file = satdump::processing::is_processing ? argv[3] : "";
    std::string output_file = satdump::processing::is_processing ? argv[4] : "";

    // Parse flags
    nlohmann::json parameters = satdump::processing::is_processing ? parse_common_flags(argc - 5, &argv[5]) : "";

    // logger->warn("\n" + parameters.dump(4));
    // exit(0);

    // Init SatDump
    satdump::initSatdump();

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

    // Init UI
    satdump::initMainUI();

    if (satdump::processing::is_processing)
    {
        satdump::ui_thread_pool.push([&](int)
                                     { satdump::processing::process(downlink_pipeline, input_level, input_file, output_file, parameters); });
    }

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
        satdump::renderMainUI(wwidth, wheight);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        if (satdump::light_theme)
            glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        else
            glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
        // glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    satdump::exitMainUI();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    logger->info("UI Exit");

    // If we're doing live processing, we want this to kill all threads quickly. Hence don't call destructors
    if (satdump::processing::is_processing)
#ifdef __APPLE__
        exit(0);
#else
        quick_exit(0);
#endif

    satdump::ui_thread_pool.stop();

    for (int i = 0; i < satdump::ui_thread_pool.size(); i++)
    {
        if (satdump::ui_thread_pool.get_thread(i).joinable())
            satdump::ui_thread_pool.get_thread(i).join();
    }

    logger->info("Exiting!");
}
