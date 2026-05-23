#pragma once

#include "ggak_product.h"
#include "handlers/product/product_handler.h"

namespace elektro_arktika
{
    namespace ggak
    {
        class GGAKProductHandler : public satdump::handlers::ProductHandler
        {
        public:
            GGAKProductHandler(std::shared_ptr<satdump::products::Product> p, bool dataset_mode = false);
            ~GGAKProductHandler();

            GGAKProduct *ggak_product;

            enum ViewMode
            {
                VIEW_MAGNETOMETER,
                VIEW_PARTICLES,
                VIEW_SUBPACKETS,
                VIEW_SUMMARY,
            };

            ViewMode current_view = VIEW_MAGNETOMETER;

            bool mag_overlay = true;
            bool mag_show_btotal = true;
            bool mag_show_bx = true;
            bool mag_show_by = true;
            bool mag_show_bz = true;
            bool mag_show_voltage = false;

            bool particle_overlay = true;
            std::vector<int> particle_visible;

            int selected_subpacket_group = 0;

            struct SubpacketGroupInfo
            {
                std::string name;
                std::vector<int> channel_indices;
            };
            std::vector<SubpacketGroupInfo> subpacket_group_info;

            void do_process() override {}
            void drawMenu() override;
            void drawContents(ImVec2 win_size) override;

            std::string getID() override { return "ggak_product_handler"; }

        private:
            void drawMagnetometerPlots(ImVec2 win_size);
            void drawParticlePlots(ImVec2 win_size);
            void drawSubpacketPlots(ImVec2 win_size);
            void drawSummary(ImVec2 win_size);

            void drawChannelPlot(const GGAKChannel &ch, ImVec2 size, const char *y_label = nullptr);

            std::vector<std::string> particle_labels;
            std::vector<std::string> subpacket_rb_labels;
        };
    } // namespace ggak
} // namespace elektro_arktika
