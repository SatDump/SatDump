#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "viewer/viewer.h"
#include "imgui/pfd/widget.h"
#include "tree.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_image.h"

namespace satdump
{
    class ViewerHandler
    {
    protected:
        TreeDrawer tree_local;

    public:
        Products *products;
        nlohmann::ordered_json instrument_cfg;

        virtual void init() = 0;
        virtual void drawMenu() = 0;
        virtual void drawContents(ImVec2 win_size) = 0;
        virtual float drawTreeMenu() = 0;

        // Projection stuff
        virtual bool canBeProjected() { return false; } // Can the current product be projected?
        virtual bool hasProjection() { return false; }  // Does it have a projection ready?
        virtual bool shouldProject() { return false; }  // Should it be projected?
        virtual void updateProjection(int /*width*/, int /*height*/, nlohmann::json /*settings*/, float * /*progess*/) {}
        virtual image::Image<uint16_t> &getProjection() { throw std::runtime_error("Did you check this could be projected!?"); }
        virtual unsigned int getPreviewImageTexture() { return 0; }
        virtual void setShouldProject(bool) {}

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

        bool dragging_panel = false;
        float panel_ratio = 0.23;

        FileSelectWidget select_dataset_dialog = FileSelectWidget("Dataset", "Select Dataset");
        FileSelectWidget select_products_dialog = FileSelectWidget("Products", "Select Products");

        struct ProductsHandler
        {
            std::shared_ptr<Products> products;
            std::shared_ptr<ViewerHandler> handler;
            std::string dataset_name = "";

            bool marked_for_close = false;
        };

        ImRect renderHandler(ProductsHandler &handler, int index);

        virtual void drawPanel();
        virtual void drawContent();

        std::vector<std::string> opened_datasets;

        std::vector<ProductsHandler> products_and_handlers;
        int products_cnt_in_dataset(std::string dataset_name)
        {
            int cnt = 0;
            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                if (products_and_handlers[i].dataset_name == dataset_name)
                    cnt++;
            return cnt;
        }

        int current_handler_id = 0;

        int current_selected_tab = 0;

    public: // Projection UI stuff
        image::Image<uint16_t> projected_image_result;
        ImageViewWidget projection_image_widget;
        void drawProjectionPanel();

        struct ExternalProjSource
        {
            std::string name;
            nlohmann::json cfg;
            nlohmann::json path;
            image::Image<uint16_t> img;
        };
        int selected_external_type = 0;
        std::vector<ExternalProjSource> projections_external_sources;
        image::Image<uint16_t> projectExternal(int width, int height, nlohmann::json tcfg, ExternalProjSource &ep, float *progress);

        int projection_osm_zoom = 3;
        bool is_opening_layer = false;
        bool projections_should_refresh = false;
        bool already_has_osm_layer = false;

        struct ProjectionLayer
        {
            std::string name;
            int type; // 0 = products, 1 = external
            ProductsHandler *viewer_prods;
            ExternalProjSource *external;
            float opacity = 100;
            bool enabled = true;
            float progress = 0;

            unsigned int non_products_texid = 0;
            unsigned int getPreview()
            {
                if (type == 0)
                {
                    return viewer_prods->handler->getPreviewImageTexture();
                }
                else if (type == 1)
                {
                    if (non_products_texid == 0)
                    {
                        non_products_texid = makeImageTexture();
                        auto img = external->img.resize_to(100, 100).to8bits();
                        uint32_t *tmp_rgba = new uint32_t[img.width() * img.height()];
                        uchar_to_rgba(img.data(), tmp_rgba, img.width() * img.height(), img.channels());
                        updateImageTexture(non_products_texid, tmp_rgba, img.width(), img.height());
                        delete[] tmp_rgba;
                    }
                    return non_products_texid;
                }
                else
                {
                    return 0;
                }
            }
        };
        std::vector<ProjectionLayer> projection_layers;

        std::string projection_new_layer_name = "Ext Layer";
        FileSelectWidget projection_new_layer_file = FileSelectWidget("Image (Equ)", "Select Equirectangular Image");
        FileSelectWidget projection_new_layer_cfg = FileSelectWidget("Config (JSON)", "Select Projection Config");

        void refreshProjectionLayers();

        bool projections_draw_map_overlay = true;
        bool projections_draw_cities_overlay = true;
        float projections_cities_scale = 0.5;

        ImVec4 color_borders = {0, 1, 0, 1};
        ImVec4 color_cities = {1, 0, 0, 1};

        int projections_current_selected_proj = 0;
        /////////////
        float projections_equirectangular_tl_lon = -180;
        float projections_equirectangular_tl_lat = 90;
        float projections_equirectangular_br_lon = 180;
        float projections_equirectangular_br_lat = -90;
        /////////////
        float projections_stereo_center_lon = 0;
        float projections_stereo_center_lat = 0;
        float projections_stereo_scale = 2;
        /////////////
        float projections_tpers_lon = 0;
        float projections_tpers_lat = 0;
        float projections_tpers_alt = 30000;
        float projections_tpers_ang = 0;
        float projections_tpers_azi = 0;

        bool projections_are_generating = false;
        void generateProjectionImage();

        std::string mapurl = "http://tile.openstreetmap.org/";

        // Settings
        int projections_image_width = 2048;
        int projections_image_height = 1024;

        int projections_mode_radio = 0;

    public:
        ViewerApplication();
        ~ViewerApplication();
        void loadDatasetInViewer(std::string path);
        void loadProductsInViewer(std::string path, std::string dataset_name = "");

    public:
        static std::string getID() { return "viewer"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<ViewerApplication>(); }
    };
};