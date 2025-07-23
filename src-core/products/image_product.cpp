#include "image_product.h"
#include "core/config.h"
#include "core/exception.h"
#include "image/image_utils.h"
#include "image/io.h"
#include "logger.h"
#include "utils/http.h"

#ifdef __ANDROID__
#include <android_native_app_glue.h>
extern struct android_app *g_App;
#endif

namespace satdump
{
    namespace products
    {
        void ImageProduct::save(std::string directory)
        {
            type = "image";

            if (save_as_matrix)
                contents["save_as_matrix"] = save_as_matrix;

            std::string image_format;
            try
            {
                image_format = satdump_cfg.getValueFromSatDumpGeneral<std::string>("product_format");
            }
            catch (std::exception &e)
            {
                logger->error("Product format not specified, using PNG! %s", e.what());
                image_format = "png";
            }

            std::mutex savemtx;
#pragma omp parallel for
            for (int64_t c = 0; c < (int64_t)images.size(); c++)
            {
                savemtx.lock();
                if (images[c].filename.find(".png") == std::string::npos && images[c].filename.find(".jpeg") == std::string::npos && images[c].filename.find(".jpg") == std::string::npos &&
                    images[c].filename.find(".j2k") == std::string::npos && images[c].filename.find(".tiff") == std::string::npos && images[c].filename.find(".tif") == std::string::npos &&
                    images[c].filename.find(".qoi") == std::string::npos && images[c].filename.find(".pbm") == std::string::npos)
                    images[c].filename += "." + image_format;
                else if (!d_no_not_save_images)
                    logger->trace("Image format was specified in product call. Not supposed to happen!");

                //// META
                contents["images"][c]["file"] = images[c].filename;
                contents["images"][c]["name"] = images[c].channel_name;

                contents["images"][c]["abs_index"] = images[c].abs_index;
                contents["images"][c]["bit_depth"] = images[c].bit_depth;
                contents["images"][c]["transform"] = images[c].ch_transform;

                if (images[c].wavenumber != -1)
                    contents["images"][c]["wavenumber"] = images[c].wavenumber;
                if (images[c].calibration_type != "")
                    contents["images"][c]["calibration_type"] = images[c].calibration_type;
                //// META

                savemtx.unlock();
                if (!save_as_matrix && !d_no_not_save_images)
                    image::save_img(images[c].image, directory + "/" + images[c].filename);
            }

            if (save_as_matrix && !images.empty())
            {
                int size = ceil(sqrt(images.size()));
                logger->debug("Using size %d", size);
                image::Image image_all = image::make_manyimg_composite(size, size, images.size(), [this](int c) { return images[c].image; });
                image::save_img(image_all, directory + "/" + images[0].filename);
                savemtx.lock();
                contents["img_matrix_size"] = size;
                savemtx.unlock();
            }

            Product::save(directory);
        }

        void ImageProduct::load(std::string file)
        {
            Product::load(file);
            std::string directory = std::filesystem::path(file).parent_path().string();

            if (contents.contains("save_as_matrix"))
                save_as_matrix = contents["save_as_matrix"].get<bool>();

#ifdef __ANDROID__
            JavaVM *java_vm = g_App->activity->vm;
            JNIEnv *java_env = NULL;

            jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
            if (jni_return == JNI_ERR)
                throw std::runtime_error("Could not get JNI environement");

            jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
            if (jni_return != JNI_OK)
                throw std::runtime_error("Could not attach to thread");

            jclass activityClass = java_env->FindClass("android/app/NativeActivity");
            jmethodID getCacheDir = java_env->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
            jobject cache_dir = java_env->CallObjectMethod(g_App->activity->clazz, getCacheDir);

            jclass fileClass = java_env->FindClass("java/io/File");
            jmethodID getPath = java_env->GetMethodID(fileClass, "getPath", "()Ljava/lang/String;");
            jstring path_string = (jstring)java_env->CallObjectMethod(cache_dir, getPath);

            const char *path_chars = java_env->GetStringUTFChars(path_string, NULL);
            std::string tmp_path(path_chars);

            java_env->ReleaseStringUTFChars(path_string, path_chars);
            jni_return = java_vm->DetachCurrentThread();
            if (jni_return != JNI_OK)
                throw std::runtime_error("Could not detach from thread");
#else
            std::string tmp_path = std::filesystem::temp_directory_path().string();
#endif

            image::Image img_matrix;
            if (save_as_matrix)
            {
                if (file.find("http") == 0)
                {
                    std::string res;
                    if (perform_http_request(directory + "/" + contents["images"][0]["file"].get<std::string>(), res))
                        throw std::runtime_error("Could not download from : " + directory + "/" + contents["images"][0]["file"].get<std::string>());
                    std::ofstream(tmp_path + "/satdumpdltmp.tmp", std::ios::binary).write((char *)res.data(), res.size());
                    image::load_img(img_matrix, tmp_path + "/satdumpdltmp.tmp");
                    if (std::filesystem::exists(tmp_path + "/satdumpdltmp.tmp"))
                        std::filesystem::remove(tmp_path + "/satdumpdltmp.tmp");
                }
                else if (!d_no_not_load_images)
                {
                    if (std::filesystem::exists(directory + "/" + contents["images"][0]["file"].get<std::string>()))
                        image::load_img(img_matrix, directory + "/" + contents["images"][0]["file"].get<std::string>());
                }
            }

            for (size_t c = 0; c < contents["images"].size(); c++)
            {
                ImageHolder img_holder;
                if (!save_as_matrix)
                    logger->info("Loading " + contents["images"][c]["file"].get<std::string>());

                //// META
                //                printf("\n%s\n", contents["images"][c].dump(4).c_str());

                img_holder.filename = contents["images"][c]["file"].get<std::string>();
                img_holder.channel_name = contents["images"][c]["name"].get<std::string>();

                img_holder.abs_index = contents["images"][c]["abs_index"];
                img_holder.bit_depth = contents["images"][c]["bit_depth"];
                img_holder.ch_transform = contents["images"][c]["transform"];

                if (contents["images"][c].contains("wavenumber"))
                    img_holder.wavenumber = contents["images"][c]["wavenumber"];
                if (contents["images"][c].contains("calibration_type"))
                    img_holder.calibration_type = contents["images"][c]["calibration_type"];
                //// META

                if (!save_as_matrix)
                {
                    if (file.find("http") == 0)
                    {
                        std::string res;
                        if (perform_http_request(directory + "/" + contents["images"][c]["file"].get<std::string>(), res))
                            throw std::runtime_error("Could not download from : " + directory + "/" + contents["images"][c]["file"].get<std::string>());
                        std::ofstream(tmp_path + "/satdumpdltmp.tmp", std::ios::binary).write((char *)res.data(), res.size());
                        image::load_img(img_holder.image, tmp_path + "/satdumpdltmp.tmp");
                        if (std::filesystem::exists(tmp_path + "/satdumpdltmp.tmp"))
                            std::filesystem::remove(tmp_path + "/satdumpdltmp.tmp");
                    }
                    else if (!d_no_not_load_images)
                    {
                        if (std::filesystem::exists(directory + "/" + contents["images"][c]["file"].get<std::string>()))
                            image::load_img(img_holder.image, directory + "/" + contents["images"][c]["file"].get<std::string>());
                    }
                }
                else
                {
                    int m_size = contents["img_matrix_size"].get<int>();
                    int img_width = img_matrix.width() / m_size;
                    int img_height = img_matrix.height() / m_size;
                    int pos_x = c % m_size;
                    int pos_y = c / m_size;

                    int px_pos_x = pos_x * img_width;
                    int px_pos_y = pos_y * img_height;

                    img_holder.image = img_matrix.crop_to(px_pos_x, px_pos_y, px_pos_x + img_width, px_pos_y + img_height);
                }

                images.push_back(img_holder);
            }

            if (images.size() == 0)
                throw satdump_exception("ImageProduct with no images. Shouldn't happen!");
        }

        ImageProduct::~ImageProduct() {}
    } // namespace products
} // namespace satdump