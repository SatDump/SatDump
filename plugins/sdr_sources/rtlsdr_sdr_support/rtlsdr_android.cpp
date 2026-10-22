#ifdef __ANDROID__
#include "rtlsdr_android.h"
#include "core/exception.h"
#include <android_native_app_glue.h>
#include <libusb.h>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <thread>
#include <chrono>
#include "rtl_android_catalog.h"

namespace rtl_android
{
    std::vector<Device> devices()
    {
        libusb_context *ctx = nullptr;
        int rc = libusb_init(&ctx);
        if (rc != 0)
            throw satdump_exception("Could not enumerate RTL-SDR USB devices: " + std::to_string(rc));
        auto cleanup = std::unique_ptr<libusb_context, decltype(&libusb_exit)>(ctx, libusb_exit);
        libusb_device **list = nullptr;
        auto count = libusb_get_device_list(ctx, &list);
        if (count < 0)
            throw satdump_exception("Could not list RTL-SDR USB devices: " + std::to_string(count));
        std::vector<Device> result;
        for (ssize_t i = 0; i < count; ++i)
        {
            libusb_device_descriptor desc{};
            if (libusb_get_device_descriptor(list[i], &desc) != 0)
                continue;
            for (const auto &known : known_devices)
                if (desc.idVendor == known.vid && desc.idProduct == known.pid)
                {
                    char path[64];
                    std::snprintf(path, sizeof(path), "/dev/bus/usb/%03u/%03u",
                                  libusb_get_bus_number(list[i]), libusb_get_device_address(list[i]));
                    int index = result.size();
                    result.push_back({path, std::string(known.name) + " #" + std::to_string(index), index});
                    break;
                }
        }
        libusb_free_device_list(list, 1);
        return result;
    }

    void wait_permission(const std::string &path, const std::function<bool()> &cancelled)
    {
        bool request = true;
        while (!cancelled())
        {
            if (permission(path, request))
            {
                if (cancelled()) break;
                return;
            }
            request = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        throw satdump_exception("RTL-SDR USB permission wait was cancelled");
    }

    // LIBUSB_ANDROID_JVM_PTR actually contains android_app*, as expected by the
    // pinned libusb/core.c. Keep that contract and UsbDeviceConnection ownership.
    class ActivityJNI
    {
    public:
        android_app *app = nullptr;
        JNIEnv *env = nullptr;
        bool attached = false;
        ActivityJNI()
        {
            const char *value = std::getenv("LIBUSB_ANDROID_JVM_PTR");
            if (value) app = reinterpret_cast<android_app *>(std::strtoull(value, nullptr, 10));
            if (!app) throw satdump_exception("Android USB activity is unavailable");
            int rc = app->activity->vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
            if (rc == JNI_EDETACHED)
            {
                if (app->activity->vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
                    throw satdump_exception("Could not attach Android USB thread");
                attached = true;
            }
            else if (rc != JNI_OK) throw satdump_exception("Could not access Android USB JNI");
            if (env->PushLocalFrame(8) != JNI_OK)
            {
                env->ExceptionClear();
                if (attached) app->activity->vm->DetachCurrentThread();
                throw satdump_exception("Could not allocate Android USB JNI frame");
            }
        }
        ~ActivityJNI()
        {
            env->PopLocalFrame(nullptr);
            if (attached) app->activity->vm->DetachCurrentThread();
        }
    };

    bool permission(const std::string &path, bool request)
    {
        ActivityJNI jni;
        auto clazz = jni.env->GetObjectClass(jni.app->activity->clazz);
        auto method = jni.env->GetMethodID(clazz, "rtlUsbPermission", "(Ljava/lang/String;Z)I");
        if (!method || jni.env->ExceptionCheck())
        {
            jni.env->ExceptionClear();
            throw satdump_exception("Android RTL-SDR permission bridge is unavailable");
        }
        auto name = jni.env->NewStringUTF(path.c_str());
        int state = jni.env->CallIntMethod(jni.app->activity->clazz, method, name, request);
        if (jni.env->ExceptionCheck())
        {
            jni.env->ExceptionClear();
            throw satdump_exception("Android RTL-SDR USB permission check failed");
        }
        if (state == 1) return true;
        if (state == 0) return false;
        if (state == -1) throw satdump_exception("USB permission for RTL-SDR was denied");
        if (state == -2) throw satdump_exception("RTL-SDR dongle was disconnected");
        throw satdump_exception("Could not request USB permission for RTL-SDR");
    }
}
#endif
