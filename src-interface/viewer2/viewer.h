#pragma once

#include "../app.h"
#include "products2/product.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/pfd/widget.h"
#include <mutex>
// #include "common/widgets/image_view.h"
// #include "imgui/imgui_image.h"
// #include "nlohmann/json_utils.h"
//  #include "common/overlay_handler.h"

#include "viewer/tree.h"

namespace satdump
{
    class ViewerHandler2
    {
    protected:
        TreeDrawer tree_local;

    public:
        products::Product *product;
        nlohmann::ordered_json instrument_cfg;

        virtual void init() = 0;
        virtual void drawMenu() = 0;
        virtual void drawContents(ImVec2 win_size) = 0;
        virtual float drawTreeMenu() = 0;

        static std::string getID();
        static std::shared_ptr<ViewerHandler2> getInstance();
    };

    extern std::map<std::string, std::function<std::shared_ptr<ViewerHandler2>()>> viewer_handlers_registry2; // TODOREWORK
    void registerViewerHandlers2();

    class ViewerApplication2 : public Application
    {
    public:
        struct RenderLoadMenuElementsEvent
        {
        };

    protected:
        const std::string app_id;
        virtual void drawUI();

        float panel_ratio = 0.23;
        float last_width = -1.0f;

        FileSelectWidget select_dataset_products_dialog = FileSelectWidget("Dataset/Products", "Select Dataset/Products", false, true);

        struct ProductsHandler2
        {
            std::shared_ptr<products::Product> products;
            std::shared_ptr<ViewerHandler2> handler;
            std::string dataset_name = "";

            bool marked_for_close = false;

            ProductsHandler2() {}

            ProductsHandler2(std::shared_ptr<products::Product> products,
                             std::shared_ptr<ViewerHandler2> handler,
                             std::string dataset_name)
                : products(products),
                  handler(handler),
                  dataset_name(dataset_name) {}
        };

        ImRect renderHandler(ProductsHandler2 &handler, int index);

        virtual void drawPanel();
        virtual void drawContent();

        std::vector<std::string> opened_datasets;
        std::mutex product_handler_mutex;
        std::vector<std::shared_ptr<ProductsHandler2>> products_and_handlers;
        int products_cnt_in_dataset(std::string dataset_name)
        {
            int cnt = 0;
            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                if (products_and_handlers[i]->dataset_name == dataset_name)
                    cnt++;
            return cnt;
        }

        int current_handler_id = 0;

        int current_selected_tab = 0;

    public:
        ViewerApplication2();
        ~ViewerApplication2();
        void loadDatasetInViewer(std::string path);
        void loadProductsInViewer(std::string path, std::string dataset_name = "");

    public:
        static std::string getID() { return "viewer2"; }
        std::string get_name() { return "Viewer2"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<ViewerApplication2>(); }
    };
};