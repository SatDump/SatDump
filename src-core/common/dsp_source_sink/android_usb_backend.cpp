#include "android_usb_backend.h"

#ifdef __ANDROID__
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include "logger.h"

extern struct android_app *g_App;

int getDeviceFD(int &vid, int &pid, const std::vector<DevVIDPID> allowedVidPids, std::string &path)
{
    JavaVM *java_vm = g_App->activity->vm;
    JNIEnv *java_env = NULL;

    jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        return -1;

    jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
    if (jni_return != JNI_OK)
        return -1;

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == NULL)
        return -1;

    jfieldID fd_field_id = java_env->GetFieldID(native_activity_clazz, "SDR_FD", "I");
    jfieldID vid_field_id = java_env->GetFieldID(native_activity_clazz, "SDR_VID", "I");
    jfieldID pid_field_id = java_env->GetFieldID(native_activity_clazz, "SDR_PID", "I");
    jfieldID pat_field_id = java_env->GetFieldID(native_activity_clazz, "SDR_PATH", "Ljava/lang/String;");

    if (!vid_field_id || !vid_field_id || !pid_field_id)
        return -1;

    int fd = java_env->GetIntField(g_App->activity->clazz, fd_field_id);
    vid = java_env->GetIntField(g_App->activity->clazz, vid_field_id);
    pid = java_env->GetIntField(g_App->activity->clazz, pid_field_id);
    jstring jstr = (jstring)java_env->GetObjectField(g_App->activity->clazz, pat_field_id);

    const char *_str = java_env->GetStringUTFChars(jstr, NULL);
    path = std::string(_str);
    java_env->ReleaseStringUTFChars(jstr, _str);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        return -1;

    // If no vid/pid was given, just return successfully
    if (allowedVidPids.empty())
    {
        return fd;
    }

    // Otherwise, check that the vid/pid combo is allowed
    for (auto const &vp : allowedVidPids)
    {
        if (vp.vid != vid || vp.pid != pid)
        {
            continue;
        }
        return fd;
    }

    return -1;
}
#endif
