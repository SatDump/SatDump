#include "image_product_handler.h"

#include "imgui/imgui_stdlib.h"
#include "products2/image/product_equation.h"
#include "common/image/processing.h" // TODOREWORK

namespace satdump
{
    namespace viewer
    {
        ImageProductHandler::ImageProductHandler(std::shared_ptr<products::Product> p)
            : ProductHandler(p)
        {
            product = (products::ImageProduct *)ProductHandler::product.get();
        }

        ImageProductHandler::~ImageProductHandler()
        {
        }

        void ImageProductHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("ImageProduct TODOREWORK"))
            {
                ImGui::InputText("##equation", &equation);
                if (ImGui::Button("Apply"))
                {
                    auto fun = [this]()
                    {
                        //  logger->critical("PID %llu", getpid());
                        auto img = products::generate_equation_product_composite(product, equation, &progress);
                        image::equalize(img, true);
                        img_handler.updateImage(img); //  image_view.update(img);
                    };
                    if (wip_thread)
                    {
                        if (wip_thread->joinable())
                            wip_thread->join();
                        wip_thread.reset();
                    }
                    wip_thread = std::make_shared<std::thread>(fun);
                }
                ImGui::ProgressBar(progress);

                for (auto &ch : product->images)
                {
                    ImGui::Separator();
                    ImGui::Text("Channel %s", ch.channel_name.c_str());
                    ch.ch_transform.render();
                }
            }

            img_handler.drawMenu();
        }

        void ImageProductHandler::drawContents(ImVec2 win_size)
        {
            img_handler.drawContents(win_size); //  image_view.draw(win_size);
        }
    }
}
