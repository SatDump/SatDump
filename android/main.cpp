#include <unistd.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include "backend.h"
#include "main_ui.h"
#include "logger.h"
#include "init.h"
#include "loader/loader.h"

// Data
EGLDisplay g_EglDisplay = EGL_NO_DISPLAY;
EGLSurface g_EglSurface = EGL_NO_SURFACE;
static EGLContext g_EglContext = EGL_NO_CONTEXT;
static bool g_Initialized = false;
static char g_LogTag[] = "SatDump";
extern struct android_app *g_App;
extern std::string android_plugins_dir;
bool was_init = false;

// Forward declarations of helper functions
static int ShowSoftKeyboardInput();
static int HideSoftKeyboardInput();
static int PollUnicodeChars();
static int GetAssetData(const char *filename, void **out_data);
void bindImageTextureFunctions();

void init(struct android_app *app)
{
    setenv("LIBUSB_ANDROID_JVM_PTR", std::to_string((size_t)app).c_str(), true);

    if (g_Initialized)
        return;

    g_App = app;
    ANativeWindow_acquire(g_App->window);

    // Initialize EGL
    // This is mostly boilerplate code for EGL...
    {
        g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (g_EglDisplay == EGL_NO_DISPLAY)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");

        if (eglInitialize(g_EglDisplay, 0, 0) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglInitialize() returned with an error");

        const EGLint egl_attributes[] = {EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE};
        EGLint num_configs = 0;
        if (eglChooseConfig(g_EglDisplay, egl_attributes, nullptr, 0, &num_configs) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned with an error");
        if (num_configs == 0)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned 0 matching config");

        // Get the first matching config
        EGLConfig egl_config;
        eglChooseConfig(g_EglDisplay, egl_attributes, &egl_config, 1, &num_configs);
        EGLint egl_format;
        eglGetConfigAttrib(g_EglDisplay, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
        ANativeWindow_setBuffersGeometry(g_App->window, 0, 0, egl_format);

        const EGLint egl_context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        g_EglContext = eglCreateContext(g_EglDisplay, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

        if (g_EglContext == EGL_NO_CONTEXT)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglCreateContext() returned EGL_NO_CONTEXT");

        g_EglSurface = eglCreateWindowSurface(g_EglDisplay, egl_config, g_App->window, NULL);
        eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext);
    }

    if (!was_init)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

        // Disable loading/saving of .ini file from disk.
        // FIXME: Consider using LoadIniSettingsFromMemory() / SaveIniSettingsToMemory() to save in appropriate location for Android.
        io.IniFilename = NULL;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplAndroid_Init(g_App->window);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    if (!was_init)
    {

        // Setup Dear ImGui style
        // ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();

        initLogger();
        style::setFonts(backend::device_scale);
        HideSoftKeyboardInput();
        eglSwapInterval(g_EglDisplay, 0);
        std::shared_ptr<satdump::LoadingScreenSink> loading_screen_sink = std::make_shared<satdump::LoadingScreenSink>();
        logger->add_sink(loading_screen_sink);

        satdump::tle_do_update_on_init = false;
        satdump::initSatdump();
        satdump::initMainUI();

        //Shut down loading screen
        logger->del_sink(loading_screen_sink);
        loading_screen_sink.reset();

        //Set font again to adjust for DPI
        eglSwapInterval(g_EglDisplay, 1);

        // TLE
        satdump::ui_thread_pool.push([&](int) { satdump::autoUpdateTLE(satdump::user_path + "/satdump_tles.txt"); });

        was_init = true;
    }
    else
        HideSoftKeyboardInput();

    g_Initialized = true;
}

void tick()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_EglDisplay == EGL_NO_DISPLAY)
        return;

    // Poll Unicode characters via JNI
    // FIXME: do not call this every frame because of JNI overhead
    PollUnicodeChars();

    // Open on-screen (soft) input if requested by Dear ImGui
    static bool WantTextInputLast = false;
    if (io.WantTextInput && !WantTextInputLast)
        ShowSoftKeyboardInput();
    if (!io.WantTextInput && WantTextInputLast)
        HideSoftKeyboardInput();
    WantTextInputLast = io.WantTextInput;

    // Rendering
    satdump::renderMainUI();
}

void shutdown()
{
    if (!g_Initialized)
        return;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    // ImGui::DestroyContext();

    if (g_EglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (g_EglContext != EGL_NO_CONTEXT)
            eglDestroyContext(g_EglDisplay, g_EglContext);

        if (g_EglSurface != EGL_NO_SURFACE)
            eglDestroySurface(g_EglDisplay, g_EglSurface);

        eglTerminate(g_EglDisplay);
    }

    g_EglDisplay = EGL_NO_DISPLAY;
    g_EglContext = EGL_NO_CONTEXT;
    g_EglSurface = EGL_NO_SURFACE;
    ANativeWindow_release(g_App->window);

    g_Initialized = false;
}

static void handleAppCmd(struct android_app *app, int32_t appCmd)
{
    switch (appCmd)
    {
    case APP_CMD_INIT_WINDOW:
        init(app);
        break;
    case APP_CMD_TERM_WINDOW:
    case APP_CMD_PAUSE:
        shutdown();
        break;
    case APP_CMD_SAVE_STATE:
        satdump::recorder_app->save_settings();
        satdump::viewer_app->save_settings();
        satdump::config::saveUserConfig();
        break;
    }
}

static int32_t handleInputEvent(struct android_app *app, AInputEvent *inputEvent)
{
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
}

std::string getAppFilesDir(struct android_app *app)
{
    JavaVM *java_vm = app->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        throw std::runtime_error("Could not get JNI environement");

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not attach to thread");

    jclass native_activity_clazz = java_env->GetObjectClass(app->activity->clazz);
    if (native_activity_clazz == NULL)
        throw std::runtime_error("Could not get MainActivity class");

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "getAppDir", "()Ljava/lang/String;");
    if (method_id == NULL)
        throw std::runtime_error("Could not get methode ID");

    jstring jstr = (jstring)java_env->CallObjectMethod(app->activity->clazz, method_id);

    const char *_str = java_env->GetStringUTFChars(jstr, NULL);
    std::string str(_str);
    java_env->ReleaseStringUTFChars(jstr, _str);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not detach from thread");

    return str;
}

std::string getPluginsDir(struct android_app *app)
{
    JavaVM *java_vm = app->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        throw std::runtime_error("Could not get JNI environement");

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not attach to thread");

    jclass native_activity_clazz = java_env->GetObjectClass(app->activity->clazz);
    if (native_activity_clazz == NULL)
        throw std::runtime_error("Could not get MainActivity class");

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "get_plugins_directory", "()Ljava/lang/String;");
    if (method_id == NULL)
        throw std::runtime_error("Could not get methode ID");

    jstring jstr = (jstring)java_env->CallObjectMethod(app->activity->clazz, method_id);

    const char *_str = java_env->GetStringUTFChars(jstr, NULL);
    std::string str(_str);
    java_env->ReleaseStringUTFChars(jstr, _str);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not detach from thread");

    return str;
}

void android_main(struct android_app *app)
{
    g_App = app;
    bindImageTextureFunctions();
    bindBackendFunctions();

    std::string path = getAppFilesDir(app);
    android_plugins_dir = getPluginsDir(app);
    chdir(path.c_str());

    app->onAppCmd = handleAppCmd;
    app->onInputEvent = handleInputEvent;

    while (true)
    {
        int out_events;
        struct android_poll_source *out_data;

        // Poll all events. If the app is not visible, this loop blocks until g_Initialized == true.
        while (ALooper_pollAll(g_Initialized ? 0 : -1, NULL, &out_events, (void **)&out_data) >= 0)
        {
            // Process one event
            if (out_data != NULL)
                out_data->process(app, out_data);

            // Exit the app by returning from within the infinite loop
            if (app->destroyRequested != 0)
            {
                // shutdown() should have been called already while processing the
                // app command APP_CMD_TERM_WINDOW. But we play save here
                if (!g_Initialized)
                    shutdown();

                return;
            }
        }

        // Initiate a new frame
        tick();
    }
}

// Unfortunately, there is no way to show the on-screen input from native code.
// Therefore, we call ShowSoftKeyboardInput() of the main activity implemented in MainActivity.kt via JNI.
static int ShowSoftKeyboardInput()
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        return -1;

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        return -2;

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        return -3;

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "showSoftInput", "()V");
    if (method_id == NULL)
        return -4;

    java_env->CallVoidMethod(g_App->activity->clazz, method_id);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        return -5;

    return 0;
}

static int HideSoftKeyboardInput()
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        return -1;

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        return -2;

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        return -3;

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "hideSoftInput", "()V");
    if (method_id == NULL)
        return -4;

    java_env->CallVoidMethod(g_App->activity->clazz, method_id);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        return -5;

    return 0;
}

// Unfortunately, the native KeyEvent implementation has no getUnicodeChar() function.
// Therefore, we implement the processing of KeyEvents in MainActivity.kt and poll
// the resulting Unicode characters here via JNI and send them to Dear ImGui.
static int PollUnicodeChars()
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        return -1;

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        return -2;

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        return -3;

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "pollUnicodeChar", "()I");
    if (method_id == NULL)
        return -4;

    // Send the actual characters to Dear ImGui
    ImGuiIO &io = ImGui::GetIO();
    jint unicode_character;
    while ((unicode_character = java_env->CallIntMethod(g_App->activity->clazz, method_id)) != 0)
    {
        if (unicode_character == 0x08) // BackSpace
        {
            io.AddKeyEvent(ImGuiKey_Backspace, true);
            io.AddKeyEvent(ImGuiKey_Backspace, false);
        }
        else
            io.AddInputCharacter(unicode_character);
    }

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        return -5;

    return 0;
}

// Helper to retrieve data placed into the assets/ directory (android/app/src/main/assets)
static int GetAssetData(const char *filename, void **outData)
{
    int num_bytes = 0;
    AAsset *asset_descriptor = AAssetManager_open(g_App->activity->assetManager, filename, AASSET_MODE_BUFFER);
    if (asset_descriptor)
    {
        num_bytes = AAsset_getLength(asset_descriptor);
        *outData = IM_ALLOC(num_bytes);
        int64_t num_bytes_read = AAsset_read(asset_descriptor, *outData, num_bytes);
        AAsset_close(asset_descriptor);
        IM_ASSERT(num_bytes_read == num_bytes);
    }
    return num_bytes;
}
