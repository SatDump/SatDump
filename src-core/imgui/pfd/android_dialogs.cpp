#include "android_dialogs.h"

#ifdef __ANDROID__
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include "logger.h"

struct android_app *g_App = nullptr;

void show_select_file_dialog()
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        throw std::runtime_error("Could not get JNI environement");

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not attach to thread");

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        throw std::runtime_error("Could not get MainActivity class");

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "select_file", "()V");
    if (method_id == NULL)
        throw std::runtime_error("Could not get methode ID");

    java_env->CallVoidMethod(g_App->activity->clazz, method_id);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not detach from thread");
}

std::string get_select_file_dialog_result()
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        throw std::runtime_error("Could not get JNI environement");

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not attach to thread");

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        throw std::runtime_error("Could not get MainActivity class");

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "select_file_get", "()Ljava/lang/String;");
    if (method_id == NULL)
        throw std::runtime_error("Could not get methode ID");

    jstring jstr = (jstring)java_env->CallObjectMethod(g_App->activity->clazz, method_id);

    const char *_str = java_env->GetStringUTFChars(jstr, NULL);
    std::string str(_str);
    java_env->ReleaseStringUTFChars(jstr, _str);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not detach from thread");

    return str;
}

void show_select_directory_dialog()
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        throw std::runtime_error("Could not get JNI environement");

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not attach to thread");

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        throw std::runtime_error("Could not get MainActivity class");

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "select_directory", "()V");
    if (method_id == NULL)
        throw std::runtime_error("Could not get methode ID");

    java_env->CallVoidMethod(g_App->activity->clazz, method_id);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not detach from thread");
}

std::string get_select_directory_dialog_result()
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        throw std::runtime_error("Could not get JNI environement");

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not attach to thread");

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        throw std::runtime_error("Could not get MainActivity class");

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "select_directory_get", "()Ljava/lang/String;");
    if (method_id == NULL)
        throw std::runtime_error("Could not get methode ID");

    jstring jstr = (jstring)java_env->CallObjectMethod(g_App->activity->clazz, method_id);

    const char *_str = java_env->GetStringUTFChars(jstr, NULL);
    std::string str(_str);
    java_env->ReleaseStringUTFChars(jstr, _str);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        throw std::runtime_error("Could not detach from thread");

    return str;
}
#endif
