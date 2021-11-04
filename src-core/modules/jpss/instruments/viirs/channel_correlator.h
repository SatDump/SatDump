#pragma once

#include "channel_reader.h"
#include <vector>

/*
Simple correlator between VIIRS channels only keeping common segments
That's required for composites and proper differential decoding
*/

namespace jpss
{
    namespace viirs
    {
        std::pair<VIIRSReader, VIIRSReader> correlateChannels(VIIRSReader segments1, VIIRSReader segments2);                                           // Correlate 2 channels
        std::tuple<VIIRSReader, VIIRSReader, VIIRSReader> correlateThreeChannels(VIIRSReader segments1, VIIRSReader segments2, VIIRSReader segments3); // Correlate 3 channels
        std::vector<VIIRSReader> correlateChannels(std::vector<VIIRSReader> channels);                                                                 // Correlate x channels
    }                                                                                                                                                  // namespace viirs
} // namespace jpss