#pragma once

/**
 * @file punctiform_product.h
 */

#include "common/geodetic/json.h"
#include "core/exception.h"
#include "product.h"

namespace satdump
{
    namespace products
    {
        /**
         * TODOREWORK
         */
        class PunctiformProduct : public Product
        {
        public:
            /**
             * @brief TODOREWORK
             */
            struct DataHolder
            {
                // TODOREWORK        int abs_index = -1;
                std::string channel_name;
                std::vector<double> timestamps;
                std::vector<geodetic::geodetic_coords_t> positions;
                std::vector<double> data;

                //                std::string calibration_type = ""; /* TODOREWORK */
                NLOHMANN_DEFINE_TYPE_INTRUSIVE(DataHolder, /*abs_index,*/ channel_name, timestamps, positions, data)
            };

            std::vector<DataHolder> data;

        public:
            // TODOREWORK
            void set_tle(nlohmann::json tle) { contents["tle"] = tle; }

            bool has_tle() { return contents.contains("tle"); }

            nlohmann::json get_tle() { return contents["tle"]; }

        private:
            void *satellite_tracker = nullptr;

        public:
            // TODOREWORK
            geodetic::geodetic_coords_t get_sample_position(int ch_index, int sample_index);

            int getChannelIndexByName(std::string name)
            {
                for (int i = 0; i < data.size(); i++)
                    if (data[i].channel_name == name)
                        return i;
                throw satdump_exception("Invalid channel for punctiform product : " + name);
            }

            DataHolder &getChannelByName(std::string name)
            {
                for (auto &d : data)
                    if (d.channel_name == name)
                        return d;
                throw satdump_exception("Invalid channel for punctiform product : " + name);
            }

        public:
            // TODOREWORK

        public:
            virtual void save(std::string directory);
            virtual void load(std::string file);

            virtual ~PunctiformProduct();
        };
    } // namespace products
} // namespace satdump