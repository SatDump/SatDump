#include "backend.h"
#include "cli/open.h"
#include "cli/recstart.h"
#include "common/cli_utils.h"
#include "common/detect_header.h"
#include "core/cli/cli.h"
#include "core/plugin.h"
#include "core/resources.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_filedrop.h"
#include "imgui/imgui_image.h"
#include "init.h"
#include "loader/loader.h"
#include "logger.h"
#include "main_ui.h"
#include "nfd.h"
// #include "processing.h"
#include "recorder/recorder.h"
#include "satdump_vars.h"
#include <GLFW/glfw3.h>
#include <exception>
#include <filesystem>
#include <signal.h>
#include <string>
#include <vector>

GLFWwindow *window;
static volatile bool signal_caught = false;
bool fallback_gl = false;

void sig_handler_ui(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
        signal_caught = true;
}

static void glfw_error_callback(int error, const char *description) { logger->error("Glfw Error " + std::to_string(error) + ": " + description); }

void window_content_scale_callback(GLFWwindow *, float xscale, float)
{
    backend::device_scale = xscale / style::macos_framebuffer_scale();
    satdump::update_ui = true;
}

void bindBaseTextureFunctions();
void bindAdvancedTextureFunctions();

void glfw_drop_callback(GLFWwindow *window, int count, const char **paths)
{
    std::vector<std::string> files;
    for (int i = 0; i < count; i++)
        files.push_back(std::string(paths[i]));
    satdump::eventBus->fire_event<satdump::imgui_utils::FileDropEvent>({files});
}

#ifdef _WIN32
#define WINVER 0x0501 // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include "fcntl.h"
#include "io.h"
#include "stdio.h"
#include "stdlib.h"
#include "windows.h"
#pragma comment(lib, "User32.lib")

// https://www.tillett.info/2013/05/13/how-to-create-a-windows-program-that-works-as-both-as-a-gui-and-console-application/
// Attach output of application to parent console
static BOOL attachOutputToConsole(void)
{
    HANDLE consoleHandleOut, consoleHandleError;

    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // Redirect unbuffered STDOUT to the console
        consoleHandleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (consoleHandleOut != INVALID_HANDLE_VALUE)
        {
            freopen("CONOUT$", "w", stdout);
            setvbuf(stdout, NULL, _IONBF, 0);
        }
        else
        {
            return FALSE;
        }
        // Redirect unbuffered STDERR to the console
        consoleHandleError = GetStdHandle(STD_ERROR_HANDLE);
        if (consoleHandleError != INVALID_HANDLE_VALUE)
        {
            freopen("CONOUT$", "w", stderr);
            setvbuf(stderr, NULL, _IONBF, 0);
        }
        else
        {
            return FALSE;
        }
        return TRUE;
    }
    // Not a console application
    return FALSE;
}
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    attachOutputToConsole();
#endif

    initLogger();

    // logger->warn("\n" + parameters.dump(4));
    // exit(0);

#if defined(__linux__) // Wayland GLFW support is frankly broken at the moment, so force X11
#if defined(GLFW_PLATFORM_X11)
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
#endif

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        logger->critical("Could not init GLFW");
        exit(1);
    }

    // GL STUFF
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif

#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4)
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "satdump");
#endif

    // Decide GL+GLSL versions
    const char *OPENGL_VERSIONS_GLSL[] = {"#version 100", "#version 130", "#version 150"};
    int selected_glsl;
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    selected_glsl = 0;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    selected_glsl = 2;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
    // GL 3.0 + GLSL 130
    selected_glsl = 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    window = glfwCreateWindow(1000, 600, satdump::getSatDumpVersionName().c_str(), nullptr, nullptr);
    if (window == nullptr)
    {
        logger->warn("Could not init GLFW Window; falling back to OpenGL 2.1...");
        glfwDefaultWindowHints();
        fallback_gl = true;
        window = glfwCreateWindow(1000, 600, std::string("SatDump v" + (std::string)satdump::SATDUMP_VERSION).c_str(), nullptr, nullptr);
        if (window == nullptr)
        {
            // TODOREWORK bring this back    pfd::message("SatDump", "Could not start SatDump UI. Please make sure your graphics card supports OpenGL 2.1 or newer", pfd::choice::ok, pfd::icon::error);
            logger->critical("Could not init GLFW Window! Exiting");
            exit(1);
        }
    }

    glfwSetDropCallback(window, glfw_drop_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Disable vsync on loading screen - not needed since frames are only pushed on log updates,
                         // and not in a loop Vsync slows down init process when items are logged quickly

    // Bind texture/backend functions
    bindBackendFunctions();
    bindBaseTextureFunctions();
#ifndef IMGUI_IMPL_OPENGL_ES2
    if (!fallback_gl)
        bindAdvancedTextureFunctions();
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.IniFilename = NULL;

    logger->debug("Starting with OpenGL %s", (char *)glGetString(GL_VERSION));
    logger->debug("Max texture size: %zu", maxTextureSize);

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifndef IMGUI_IMPL_OPENGL_ES2
    if (fallback_gl)
        ImGui_ImplOpenGL2_Init();
    else
#endif
        ImGui_ImplOpenGL3_Init(OPENGL_VERSIONS_GLSL[selected_glsl]);

    // Handle DPI changes
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    glfwSetWindowContentScaleCallback(window, window_content_scale_callback);
#endif

    // Set font
    style::setFonts(backend::device_scale);

    // Init Loading Screen
    std::shared_ptr<satdump::LoadingScreenSink> loading_screen_sink = std::make_shared<satdump::LoadingScreenSink>();
    logger->add_sink(loading_screen_sink);

    // Basic flags
    bool verbose = satdump::cli::checkVerbose(argc, argv);

    // Init SatDump, silent or verbose as requested
    if (!verbose)
        logger->set_level(slog::LOG_WARN);
    satdump::tle_do_update_on_init = false;
    satdump::initSatdump(true);
    if (!verbose)
        logger->set_level(slog::LOG_TRACE);

    // Register our own commands
    satdump::eventBus->register_handler<satdump::cli::RegisterSubcommandEvent>(
        [](const satdump::cli::RegisterSubcommandEvent &evt)
        {
            if (evt.is_gui)
            {
                evt.cmd_handlers.push_back(std::make_shared<satdump::RecStartCmdHandler>());
                evt.cmd_handlers.push_back(std::make_shared<satdump::OpenCmdHandler>());
            }
        });

    // Parse commands, if present
    satdump::cli::CommandHandler cli_handler(true);
    if (argc > 1)
    {
        int v = cli_handler.parse(argc, argv);
        if (v != 0)
            exit(v);
    }

    // Init UI
    satdump::initMainUI();

    // Shut down loading screen
    logger->del_sink(loading_screen_sink);
    loading_screen_sink.reset();
    glfwSwapInterval(1); // Enable vsync for the rest of the program

    // Attempt to run command, if present
    if (cli_handler.run())
        exit(1);

    // Attach signal
    signal(SIGINT, sig_handler_ui);
    signal(SIGTERM, sig_handler_ui);

    // Main loop
    do
    {
        satdump::renderMainUI();
    } while (!glfwWindowShouldClose(window) && !signal_caught);

    satdump::exitMainUI();

    // Cleanup
#ifndef IMGUI_IMPL_OPENGL_ES2
    if (fallback_gl)
        ImGui_ImplOpenGL2_Shutdown();
    else
#endif
        ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    logger->info("UI Exit");

    satdump::ui_thread_pool.stop();

    for (int i = 0; i < satdump::ui_thread_pool.size(); i++)
    {
        if (satdump::ui_thread_pool.get_thread(i).joinable())
            satdump::ui_thread_pool.get_thread(i).join();
    }

    logger->info("Exiting!");

#ifdef __APPLE__
    exit(0);
#else
    quick_exit(0);
#endif
}
