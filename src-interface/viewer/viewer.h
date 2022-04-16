#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace satdump
{
    class ViewerHandler
    {
    public:
        Products *products;
        nlohmann::ordered_json instrument_cfg;

        virtual void init() = 0;
        virtual void drawMenu() = 0;
        virtual void drawContents(ImVec2 win_size) = 0;
        virtual float drawTreeMenu() = 0;

        static std::string getID();
        static std::shared_ptr<ViewerHandler> getInstance();
    };

    extern std::map<std::string, std::function<std::shared_ptr<ViewerHandler>()>> viewer_handlers_registry;
    void registerViewerHandlers();

    class ViewerApplication : public Application
    {
    protected:
        const std::string app_id;
        virtual void drawUI();

        struct ProductsHandler
        {
            std::shared_ptr<Products> products;
            std::shared_ptr<ViewerHandler> handler;
            std::string dataset_name = "";
        };

        ImRect renderHandler(ProductsHandler &handler, int index);

        virtual void drawPanel();
        virtual void drawContent();

        std::vector<std::string> opened_datasets;

        std::vector<ProductsHandler> products_and_handlers;
        int products_cnt_in_dataset(std::string dataset_name)
        {
            int cnt = 0;
            for (int i = 0; i < products_and_handlers.size(); i++)
                if (products_and_handlers[i].dataset_name == dataset_name)
                    cnt++;
            return cnt;
        }

        int current_handler_id = 0;
        void loadDatasetInViewer(std::string path);
        void loadProductsInViewer(std::string path, std::string dataset_name = "");

    public:
        ViewerApplication();
        ~ViewerApplication();

    public:
        static std::string getID() { return "viewer"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<ViewerApplication>(); }
    };
};