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
#include "nlohmann/json_utils.h"
#include "common/overlay_handler.h"

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
        float last_width = -1.0f;

        FileSelectWidget select_dataset_dialog = FileSelectWidget("Dataset", "Select Dataset");
        FileSelectWidget select_products_dialog = FileSelectWidget("Products", "Select Products");

        struct ProductsHandler
        {
            std::shared_ptr<Products> products;
            std::shared_ptr<ViewerHandler> handler;
            std::string dataset_name = "";

            bool marked_for_close = false;

            ProductsHandler() {}

            ProductsHandler(std::shared_ptr<Products> products,
                            std::shared_ptr<ViewerHandler> handler,
                            std::string dataset_name)
                : products(products),
                  handler(handler),
                  dataset_name(dataset_name) {}
        };

        ImRect renderHandler(ProductsHandler &handler, int index);

        virtual void drawPanel();
        virtual void drawContent();

        std::vector<std::string> opened_datasets;
        std::mutex product_handler_mutex;
        std::vector<std::shared_ptr<ProductsHandler>> products_and_handlers;
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

        OverlayHandler projection_overlay_handler;

    public: // Projection UI stuff
        image::Image<uint16_t> projected_image_result;
        ImageViewWidget projection_image_widget;
        void drawProjectionPanel();

        int projection_osm_zoom = 3;
        bool is_opening_layer = false;

        struct ProjectionLayer
        {
            std::string name;
            image::Image<uint16_t> img;
            float opacity = 100;
            bool enabled = true;
            float progress = 0;
            bool old_algo = false;

            unsigned int preview_texid = 0;
            unsigned int getPreview()
            {
                if (preview_texid == 0)
                {
                    preview_texid = makeImageTexture();
                    auto img8 = img.resize_to(100, 100).to8bits();
                    uint32_t *tmp_rgba = new uint32_t[img8.width() * img8.height()];
                    uchar_to_rgba(img8.data(), tmp_rgba, img8.width() * img8.height(), img8.channels());
                    updateImageTexture(preview_texid, tmp_rgba, img8.width(), img8.height());
                    delete[] tmp_rgba;
                }
                return preview_texid;
            }
        };
        std::vector<ProjectionLayer> projection_layers;

        int selected_external_type = 0;
        std::string projection_new_layer_name = "Ext Layer";
        bool projection_normalize_image = false;
        FileSelectWidget projection_new_layer_file = FileSelectWidget("Image (Equ)", "Select Layer Image");
        FileSelectWidget projection_new_layer_cfg = FileSelectWidget("Config (JSON)", "Select Projection Config");
        bool projections_loading_new_layer = false;

        bool projection_auto_mode = false, projection_auto_scale_mode = false;
        double projection_autoscale_x = 0.016, projection_autoscale_y = 0.016;

        int projections_current_selected_proj = 0;
        /////////////
        float projections_equirectangular_tl_lon = -180;
        float projections_equirectangular_tl_lat = 90;
        float projections_equirectangular_br_lon = 180;
        float projections_equirectangular_br_lat = -90;
        /////////////
        float projections_utm_center_lon = 0;
        float projections_utm_offset_y = 0;
        float projections_utm_scale = 2400;
        int projections_utm_zone = 30;
        bool projections_utm_south = false;
        /////////////
        float projections_stereo_center_lon = 0;
        float projections_stereo_center_lat = 0;
        float projections_stereo_scale = 2400;
        /////////////
        float projections_tpers_lon = 0;
        float projections_tpers_lat = 0;
        float projections_tpers_alt = 30000000;
        float projections_tpers_ang = 0;
        float projections_tpers_azi = 0;
        float projections_tpers_scale = 8000;
        /////////////
        float projections_azeq_lon = 0;
        float projections_azeq_lat = 90;

        bool projections_are_generating = false;
        void generateProjectionImage();

        std::string mapurl = "http://tile.openstreetmap.org/{z}/{x}/{y}.png";
        bool urlgood = true;

        // Settings
        std::string save_type = "png";

        int projections_image_width = 2048;
        int projections_image_height = 1024;

        int projections_mode_radio = 0;

    public:
        nlohmann::json serialize_projections_config()
        {
            nlohmann::json out;

            out["projections_overlay_settings"] = projection_overlay_handler.get_config();

            out["projections_current_selected_proj"] = projections_current_selected_proj;

            out["projections_equirectangular_tl_lon"] = projections_equirectangular_tl_lon;
            out["projections_equirectangular_tl_lat"] = projections_equirectangular_tl_lat;
            out["projections_equirectangular_br_lon"] = projections_equirectangular_br_lon;
            out["projections_equirectangular_br_lat"] = projections_equirectangular_br_lat;

            out["projections_utm_center_lon"] = projections_utm_center_lon;
            out["projections_utm_offset_y"] = projections_utm_offset_y;
            out["projections_utm_scale"] = projections_utm_scale;
            out["projections_utm_zone"] = projections_utm_zone;
            out["projections_utm_south"] = projections_utm_south;

            out["projections_stereo_center_lon"] = projections_stereo_center_lon;
            out["projections_stereo_center_lat"] = projections_stereo_center_lat;
            out["projections_stereo_scale"] = projections_stereo_scale;

            out["projections_tpers_lon"] = projections_tpers_lon;
            out["projections_tpers_lat"] = projections_tpers_lat;
            out["projections_tpers_alt"] = projections_tpers_alt;
            out["projections_tpers_ang"] = projections_tpers_ang;
            out["projections_tpers_azi"] = projections_tpers_azi;
            out["projections_tpers_scale"] = projections_tpers_scale;

            out["projections_image_width"] = projections_image_width;
            out["projections_image_height"] = projections_image_height;

            out["projections_mode_radio"] = projections_mode_radio;

            return out;
        }

        void deserialize_projections_config(nlohmann::json in)
        {
            if (in.contains("projections_overlay_settings"))
                projection_overlay_handler.set_config(in["projections_overlay_settings"]);

            setValueIfExists(in["projections_current_selected_proj"], projections_current_selected_proj);

            setValueIfExists(in["projections_equirectangular_tl_lon"], projections_equirectangular_tl_lon);
            setValueIfExists(in["projections_equirectangular_tl_lat"], projections_equirectangular_tl_lat);
            setValueIfExists(in["projections_equirectangular_br_lon"], projections_equirectangular_br_lon);
            setValueIfExists(in["projections_equirectangular_br_lat"], projections_equirectangular_br_lat);

            setValueIfExists(in["projections_utm_center_lon"], projections_utm_center_lon);
            setValueIfExists(in["projections_utm_offset_y"], projections_utm_offset_y);
            setValueIfExists(in["projections_utm_scale"], projections_utm_scale);
            setValueIfExists(in["projections_utm_zone"], projections_utm_zone);
            setValueIfExists(in["projections_utm_south"], projections_utm_south);

            setValueIfExists(in["projections_stereo_center_lon"], projections_stereo_center_lon);
            setValueIfExists(in["projections_stereo_center_lat"], projections_stereo_center_lat);
            setValueIfExists(in["projections_stereo_scale"], projections_stereo_scale);

            setValueIfExists(in["projections_tpers_lon"], projections_tpers_lon);
            setValueIfExists(in["projections_tpers_lat"], projections_tpers_lat);
            setValueIfExists(in["projections_tpers_alt"], projections_tpers_alt);
            setValueIfExists(in["projections_tpers_ang"], projections_tpers_ang);
            setValueIfExists(in["projections_tpers_azi"], projections_tpers_azi);
            setValueIfExists(in["projections_tpers_scale"], projections_tpers_scale);

            setValueIfExists(in["projections_image_width"], projections_image_width);
            setValueIfExists(in["projections_image_height"], projections_image_height);

            setValueIfExists(in["projections_mode_radio"], projections_mode_radio);
        }

    public:
        ViewerApplication();
        ~ViewerApplication();
        void save_settings();
        void loadDatasetInViewer(std::string path);
        void loadProductsInViewer(std::string path, std::string dataset_name = "");

    public:
        static std::string getID() { return "viewer"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<ViewerApplication>(); }
    };
};