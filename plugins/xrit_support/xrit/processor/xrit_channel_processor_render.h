#pragma once

/**
 * @file xrit_channel_processor_render.h
 * @brief GUI Rendering for xRIT processors
 */

#include "xrit_channel_processor.h"

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief Render (just the tabs, not tabbar!) for a processor
         * @param p xRIT processor to render
         */
        bool renderTabsFromProcessor(XRITChannelProcessor *p);

        /**
         * @brief Render the tabbar & tabs for a set of processors
         * @param ps xRIT processors to render
         */
        void renderAllTabsFromProcessors(std::vector<XRITChannelProcessor *> ps);

        /**
         * @brief Render the tabbar & tabs for a set of processors, but from a map
         * @param all_processors xRIT processors to render
         */
        void renderAllTabsFromProcessors(std::map<std::string, std::shared_ptr<XRITChannelProcessor>> &all_processors);
    } // namespace xrit
} // namespace satdump