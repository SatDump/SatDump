#pragma once

/**
 * @file ggak_product_handler.h
 * @brief UI handler for GGAK space environment products in the SatDump explorer.
 */

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

            void do_process() override {}
            void drawMenu() override;
            void drawContents(ImVec2 win_size) override;

            void setConfig(nlohmann::json p) override;
            nlohmann::json getConfig() override;

            std::string getID() override { return "ggak_product_handler"; }

        private:
            GGAKProduct *ggak_product;

            std::vector<GGAKChannel> plot_mag_channels;
            std::vector<GGAKChannel> plot_particle_channels;
            std::vector<GGAKChannel> plot_subpacket_channels;

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

            int plot_id_counter = 0;

            void drawMagnetometerPlots(ImVec2 win_size);
            void drawParticlePlots(ImVec2 win_size);
            void drawSubpacketPlots(ImVec2 win_size);
            void drawSummary(ImVec2 win_size);

            void drawChannelPlot(const GGAKChannel &ch, ImVec2 size, int plot_idx, const char *y_label = nullptr);
        };
    } // namespace ggak
} // namespace elektro_arktika
