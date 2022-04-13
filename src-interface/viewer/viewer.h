#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"

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
        virtual void drawTreeMenu() = 0;

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

        virtual void drawPanel();
        virtual void drawContent();

        std::vector<std::pair<std::shared_ptr<Products>, std::shared_ptr<ViewerHandler>>> products_and_handlers;

        int current_handler_id = 0;
        void loadProductsInViewer(std::string path);

    public:
        ViewerApplication();
        ~ViewerApplication();

    public:
        static std::string getID() { return "viewer"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<ViewerApplication>(); }
    };
};