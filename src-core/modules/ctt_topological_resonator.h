#pragma once
#include "../pipeline/module.h"

namespace satdump {
    class CTTTopologicalResonator : public Module {
    public:
        CTTTopologicalResonator(std::string id) : Module(id) {}
        void process(std::vector<std::complex<float>>& input) override;
        std::string name() override { return "CTT_Φ24_Resonator"; }
    };
}
