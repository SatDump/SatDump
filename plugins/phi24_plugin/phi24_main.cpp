#include "../../src-core/plugins/plugin.h"
#include "modules/ctt_topological_resonator.h"

extern "C" {
    void init_plugin() {
        // Register the Φ-24 Resonator into the SatDump Event Bus
        satdump::register_module<satdump::CTTTopologicalResonator>("phi24_resonator");
    }
}
