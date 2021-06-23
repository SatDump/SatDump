#define __ANDROID__
//#include "live.h"
//#include "live_run.h"
#include <SDL.h>
#include "imgui/imgui.h"
#include "loggerc.h"
#include <GLES2/gl2.h>
#include "imgui_impl/imgui_impl_sdl_es2.h"
#include "imgui_impl/imgui_impl_sdl_es3.h"
#include <unistd.h>
#include <dirent.h>
#include "style.h"
#include "imgui/imgui_flags.h"
#include "credits.h"
#include "global.h"
#include "init.h"
#include "processing.h"
#include "offline.h"
#include "logger.h"
#include <filesystem>
#include "settings.h"
#include "settingsui.h"
#include "live.h"
#include "live_run.h"

#define NOWINDOW_FLAGS (long int)ImGuiWindowFlags_NoMove | (long int)ImGuiWindowFlags_NoCollapse | (long int)ImGuiWindowFlags_NoBringToFrontOnFocus /*| ImGuiWindowFlags_NoTitleBar*/ | (long int)ImGuiWindowFlags_NoResize | (long int)ImGuiWindowFlags_NoBackground

/**
 * A convenience function to create a context for the specified window
 * @param w Pointer to SDL_Window
 * @return An SDL_Context value
 */

typedef bool(initImgui_t)(SDL_Window *);
typedef bool(processEvent_t)(SDL_Event *);
typedef void(newFrame_t)(SDL_Window *);
typedef void(shutdown_t)();

static initImgui_t *initImgui;
static processEvent_t *processEvent;
static newFrame_t *newFrame;
static shutdown_t *shutdown;

static SDL_GLContext createCtx(SDL_Window *w)
{
    // Prepare and create context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GLContext ctx = SDL_GL_CreateContext(w);

    if (!ctx)
    {
        Log(LOG_ERROR) << "Could not create context! SDL reports error: " << SDL_GetError();
        return ctx;
    }

    int major, minor, mask;
    int r, g, b, a, depth;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);

    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);

    const char *mask_desc;

    if (mask & SDL_GL_CONTEXT_PROFILE_CORE)
    {
        mask_desc = "core";
    }
    else if (mask & SDL_GL_CONTEXT_PROFILE_COMPATIBILITY)
    {
        mask_desc = "compatibility";
    }
    else if (mask & SDL_GL_CONTEXT_PROFILE_ES)
    {
        mask_desc = "es";
    }
    else
    {
        mask_desc = "?";
    }

    Log(LOG_INFO) << "Got context: " << major << "." << minor << mask_desc
                  << ", R" << r << "G" << g << "B" << b << "A" << a << ", depth bits: " << depth;

    SDL_GL_MakeCurrent(w, ctx);

    if (major == 3)
    {
        Log(LOG_INFO) << "Initializing ImGui for GLES3";
        initImgui = ImGui_ImplSdlGLES3_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES3_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES3_NewFrame;
        shutdown = ImGui_ImplSdlGLES3_Shutdown;
    }
    else
    {
        Log(LOG_INFO) << "Initializing ImGui for GLES2";
        initImgui = ImGui_ImplSdlGLES2_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES2_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES2_NewFrame;
        shutdown = ImGui_ImplSdlGLES2_Shutdown;
    }

    Log(LOG_INFO) << "Finished initialization";
    return ctx;
}

#include <jni.h>
#include <android_native_app_glue.h>

std::string getFilePath()
{
    JNIEnv *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    jclass cls = env->GetObjectClass(activity);
    //jclass localcls = reinterpret_cast<jclass>(env->NewGlobalRef(cls));
    jmethodID getFilePath = env->GetMethodID(cls, "getFilePath", "()Ljava/lang/String;");
    jstring str = (jstring)env->CallObjectMethod(activity, getFilePath);
    const char *str2 = env->GetStringUTFChars(str, 0);
    return std::string(str2);
}

std::string getDirPath()
{
    JNIEnv *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    jclass cls = env->GetObjectClass(activity);
    //jclass localcls = reinterpret_cast<jclass>(env->NewGlobalRef(cls));
    jmethodID getFilePath = env->GetMethodID(cls, "getFilePath1", "()Ljava/lang/String;");
    jstring str = (jstring)env->CallObjectMethod(activity, getFilePath);
    const char *str2 = env->GetStringUTFChars(str, 0);
    return std::string(str2);
}

void bindImageTextureFunctions();

int main(int argc, char **argv)
{
    bindImageTextureFunctions();
    std::fill(error_message, &error_message[0], 0);

    uiCallList = std::make_shared<std::vector<std::shared_ptr<ProcessingModule>>>();
    uiCallListMutex = std::make_shared<std::mutex>();

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    if (argc < 2)
    {
        Log(LOG_FATAL) << "Not enough arguments! Usage: " << argv[0] << " path_to_data_dir";
        SDL_Quit();
        return 1;
    }
    if (chdir(argv[1]))
    {
        Log(LOG_ERROR) << "Could not change directory properly!";
    }
    else
    {
        dirent **namelist;
        int numdirs = scandir(".", &namelist, NULL, alphasort);
        if (numdirs < 0)
        {
            Log(LOG_ERROR) << "Could not list directory";
        }
        else
        {
            for (int dirid = 0; dirid < numdirs; ++dirid)
            {
                Log(LOG_INFO) << "Got file: " << namelist[dirid]->d_name;
            }
            free(namelist);
        }
    }

    // Create window
    Log(LOG_INFO) << "Creating SDL_Window";
    SDL_Window *window = SDL_CreateWindow("SatDump", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext ctx = createCtx(window);
    ImGui::CreateContext();
    initImgui(window);

    ImGuiIO &io = ImGui::GetIO();

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    Log(LOG_INFO) << "Entering main loop";
    {

        bool done = false;

        int deltaX = 0, deltaY = 0;
        int prevX, prevY;
        SDL_GetMouseState(&prevX, &prevY);

        int major;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);

        if (std::filesystem::exists("settings.json"))
        {
            loadSettings("settings.json");
        }

        parseSettingsOrDefaults();

        float ddpi, hdpi, vdpi;
        SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
        float dpi_scaling = ddpi / (72.f * 3);

        if (use_light_theme)
            style::setLightStyle(".", dpi_scaling);
        else
            style::setDarkStyle(".", dpi_scaling);

        initLogger();
        initSatdump();

#ifdef BUILD_LIVE
        initLive();
#endif

        bool render = true;

        while (!done)
        {
            SDL_Event e;

            deltaX = 0;
            deltaY = 0;

            float deltaZoom = 0.0f;

            while (SDL_PollEvent(&e))
            {
                bool handledByImGui = processEvent(&e);
                {
                    switch (e.type)
                    {
                    case SDL_QUIT:
                        done = true;
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        prevX = e.button.x;
                        prevY = e.button.y;
                        break;
                    case SDL_MOUSEMOTION:
                        if (e.motion.state & SDL_BUTTON_LMASK)
                        {
                            deltaX += prevX - e.motion.x;
                            deltaY += prevY - e.motion.y;
                            prevX = e.motion.x;
                            prevY = e.motion.y;
                        }
                        break;
                    case SDL_MULTIGESTURE:
                        if (e.mgesture.numFingers > 1)
                        {
                            deltaZoom += e.mgesture.dDist * 10.0f;
                        }
                        break;
                    case SDL_MOUSEWHEEL:
                        deltaZoom += e.wheel.y / 100.0f;
                        break;
                    case SDL_APP_WILLENTERBACKGROUND:
                        render = false;
                        shutdown();
                        SDL_GL_DeleteContext(ctx);
                        break;
                    case SDL_APP_WILLENTERFOREGROUND:
                        render = true;
                        ctx = createCtx(window);
                        initImgui(window);
                        //SDLActivity.createEGLSurface();
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        //logger->info("RESIZING");
                        break;
                    default:
                        break;
                    }
                }
            }
            if (io.WantTextInput)
            {
                SDL_StartTextInput();
            }
            else
            {
                SDL_StopTextInput();
            }

            if (render)
            {
                newFrame(window);

                ImGui::SetNextWindowPos({0, 0});
                int wwidth, wheight;
                SDL_GetWindowSize(window, &wwidth, &wheight);

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
                                ImGui::Text("Live processing is not yet supported on Android!");
#endif
                                ImGui::EndTabItem();
                            }
                            if (ImGui::BeginTabItem("Settings"))
                            {
                                renderSettings(wwidth, wheight);
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

                ImGui::Render();

                // Rendering
                glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x * 200, (int)ImGui::GetIO().DisplaySize.y * 200);
                glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                if (use_light_theme)
                    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
                else
                    glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
                //glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                if (major == 3)
                    ImGui_ImplSdlGLES3_RenderDrawLists(ImGui::GetDrawData());
                else
                    ImGui_ImplSdlGLES2_RenderDrawLists(ImGui::GetDrawData());

                SDL_GL_SwapWindow(window);
            }
        }
    }
    ImGui::DestroyContext();
    shutdown();
    SDL_GL_DeleteContext(ctx);
    SDL_Quit();
    return 0;
}