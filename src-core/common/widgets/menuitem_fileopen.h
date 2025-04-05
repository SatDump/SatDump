#pragma once

#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include "core/config.h"
#include "imgui/pfd/pfd_utils.h"
#include "imgui/pfd/portable-file-dialogs.h"

namespace satdump
{
    namespace widget
    {
        class MenuItemFileOpen
        {
        private:
            pfd::open_file *file_open_dialog = nullptr;
            std::string selected_path;

        public:
            bool render(std::string menu_name, std::string dialog_name, std::string default_dir, std::vector<std::string> filters)
            {
                bool disallow = file_open_dialog;
                if (disallow)
                    style::beginDisabled();
                if (ImGui::MenuItem(menu_name.c_str()))
                    file_open_dialog = new pfd::open_file(dialog_name, default_dir, filters, pfd::opt::force_path);
                if (disallow)
                    style::endDisabled();

                if (file_open_dialog && file_open_dialog->ready(0))
                {
                    selected_path = (file_open_dialog->result().size() == 0 ? "" : file_open_dialog->result()[0]);
                    delete file_open_dialog;
                    file_open_dialog = nullptr;
                    return true;
                }

                return false;
            }

            std::string getPath() { return selected_path; }
        };

        class MenuItemImageSave
        {
        private:
            bool file_save_thread_running = false;
            std::thread file_save_thread;

        public:
            void render(std::string menu_name, std::string default_name, std::string default_dir, std::string dialog_name, bool force_png = false)
            {
                bool needs_to_be_disabled = file_save_thread_running;

                if (needs_to_be_disabled)
                    style::beginDisabled();

                if (ImGui::MenuItem(menu_name.c_str()))
                {
                    auto fun = [this, default_name, default_dir, dialog_name, force_png]()
                    {
                        file_save_thread_running = true;
                        // TODOREWORK!!!!
                        std::string save_type = "png";
                        if (!force_png)
                            config::tryAssignValueFromSatdumpGeneralConfig(save_type, "image_format");
                        auto img = getimg_callback();
                        std::string saved_at = save_image_dialog(default_name, default_dir, dialog_name, img.get(), &save_type);
                        if (saved_at == "")
                            logger->info("Save cancelled");
                        else
                            logger->info("Saved current image at %s", saved_at.c_str());
                        file_save_thread_running = false;
                    };

                    if (file_save_thread.joinable())
                        file_save_thread.join();
                    file_save_thread = std::thread(fun);
                }

                if (needs_to_be_disabled)
                    style::endDisabled();
            }

            std::function<std::shared_ptr<image::Image>()> getimg_callback;
        };
    }
}