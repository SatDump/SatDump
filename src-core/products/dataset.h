#pragma once

/**
 * @file dataset.h
 * @brief Core dataset implementation
 */

#include <string>
#include <vector>

namespace satdump
{
    namespace products
    {
        /**
         * @brief SatDump dataset struct.
         *
         * This is a simple class holding information and paths to
         * several different product, coming from the same source / acquisition.
         *
         * @param satellite_name source satellite name. Should be descriptive enough, but if the specific satellite (eg, MetOp-B) is unknown, "MetOp" is enough
         * @param timestamp rough UTC Timestamp of the dataset
         * @param products_list  list of products to load, relative to this file
         */
        struct DataSet
        {
            std::string satellite_name;
            double timestamp;
            std::vector<std::string> products_list;

            /**
             * @brief Save a dataset to a file
             * @param path to the .json file
             */
            void save(std::string path);

            /**
             * @brief Load a dataset from a file
             * @param path to the .json file
             */
            void load(std::string path);
        };
    } // namespace products
} // namespace satdump
