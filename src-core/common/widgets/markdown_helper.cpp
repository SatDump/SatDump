#include "markdown_helper.h"
#include "core/style.h"
#include "logger.h"
#include "imgui/imgui_image.h"
#include "resources.h"
#include "common/image/image.h"
#include "common/image/io.h"
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__ANDROID__)
#include <android_native_app_glue.h>
extern struct android_app *g_App;
#endif

namespace widgets
{
    void MarkdownHelper::link_callback(ImGui::MarkdownLinkCallbackData data_)
    {
        std::string url(data_.link, data_.linkLength);
        if (!data_.isImage)
        {
            logger->info("Opening URL " + url);

#if defined(_WIN32)
            // ShellExecuteA(0, 0, url.c_str(), 0, 0, SW_SHOW); //TODOUWP
#elif defined(__APPLE__)
            system(std::string("open " + url).c_str());
#elif defined(__ANDROID__)
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

            jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "openURL", "(Ljava/lang/String;)V");
            if (method_id == NULL)
                throw std::runtime_error("Could not get methode ID");

            jstring jurl = java_env->NewStringUTF(url.c_str());
            java_env->CallVoidMethod(g_App->activity->clazz, method_id, jurl);

            jni_return = java_vm->DetachCurrentThread();
            if (jni_return != JNI_OK)
                throw std::runtime_error("Could not detach from thread");
#else
            if (system(std::string("xdg-open " + url).c_str()) != 0)
                logger->error("Error opening URL!");
#endif
        }
    }

    ImGui::MarkdownImageData MarkdownHelper::image_callback(ImGui::MarkdownLinkCallbackData data_)
    {
        MarkdownHelper *tthis = (MarkdownHelper *)data_.userData;

        std::string image_path(data_.link, data_.linkLength);

        ImGui::MarkdownImageData imageData;

        if (tthis->texture_buffer.count(image_path))
        {
            imageData = tthis->texture_buffer[image_path];
        }
        else
        {
            imageData.isValid = false;

            if (std::filesystem::exists(resources::getResourcePath(image_path)))
            {
                logger->trace("Loading image for markdown : " + image_path);

                image::Image img;
                image::load_img(img, resources::getResourcePath(image_path));

                intptr_t text_id = makeImageTexture(img.width(), img.height());
                uint32_t *output_buffer = new uint32_t[img.width() * img.height()];
                image::image_to_rgba(img, output_buffer);
                updateImageTexture(text_id, output_buffer, img.width(), img.height());
                delete[] output_buffer;

                imageData.isValid = true;
                imageData.useLinkCallback = false;
                imageData.user_texture_id = (ImTextureID)text_id;
                imageData.size = ImVec2(img.width(), img.height());
            }

            tthis->texture_buffer.emplace(image_path, imageData);
        }

        // For image resize when available size.x > image width, add
        ImVec2 const contentSize = ImGui::GetContentRegionAvail();
        if (imageData.size.x > contentSize.x)
        {
            float const ratio = imageData.size.y / imageData.size.x;
            imageData.size.x = contentSize.x;
            imageData.size.y = contentSize.x * ratio;
        }

        return imageData;
    }

    MarkdownHelper::MarkdownHelper()
    {
        mdConfig.linkCallback = link_callback;
        mdConfig.imageCallback = image_callback;
    }

    void MarkdownHelper::render()
    {
        mdConfig.headingFormats[0] = {style::bigFont, true};
        mdConfig.headingFormats[1] = {style::bigFont, true};
        mdConfig.headingFormats[2] = {style::baseFont, true};
        mdConfig.userData = (void *)this;
        ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
    }

    void MarkdownHelper::set_md(std::string md)
    {
        markdown_ = md;
        texture_buffer.clear();
    }
}
