#include <complex>
#include <vector>
#include <iostream>
#include <cmath>
#include "pipeline/module.h"
#include "pipeline/factory.h"
#include "nlohmann/json.hpp"

namespace satdump {
namespace pipeline {

    class CTTTopologicalResonator : public ProcessingModule {
    private:
        float phase = 0.0f;
        float freq_offset = 0.0f;
        const float alpha = 0.01f; 
        const float beta = 0.0001f; 

    public:
        CTTTopologicalResonator(std::string id, nlohmann::json config) 
            : ProcessingModule(id, id, config) {
            std::cout << "[CTT] Phi-24 Resonator: Phase-lock engine active." << std::endl;
        }

        void process() override {
            // Guarding the manifold
            if (inputs.find("in") == inputs.end() || outputs.find("out") == outputs.end()) return;

            auto& input_buffer = inputs["in"]->getBuffer<std::complex<float>>();
            auto& output_buffer = outputs["out"]->getBuffer<std::complex<float>>();

            size_t size = input_buffer.size();
            output_buffer.resize(size);

            for (size_t i = 0; i < size; ++i) {
                // Topological mixing
                std::complex<float> lo = std::exp(std::complex<float>(0, -phase));
                std::complex<float> mixed = input_buffer[i] * lo;

                // Zeta-Zero Quenching (Error Detection)
                float error = std::arg(mixed);
                
                // Feedback Loop
                phase += freq_offset + alpha * error;
                freq_offset += beta * error;

                // Wrap phase within the circle
                if (phase > (float)M_PI) phase -= 2.0f * (float)M_PI;
                if (phase < -(float)M_PI) phase += 2.0f * (float)M_PI;

                output_buffer[i] = mixed;
            }
        }

        void drawUI(bool window) override {}
        std::string getIDM() override { return "ctt_phi24_resonator"; }
        static std::string getID() { return "ctt_phi24_resonator"; }
        static nlohmann::json getParams() { return nlohmann::json::object(); }
        
        static std::shared_ptr<ProcessingModule> getInstance(std::string id, nlohmann::json config) {
            return std::make_shared<CTTTopologicalResonator>(id, config);
        }
    };

    __attribute__((constructor))
    static void register_ctt_module() {
        ProcessingModuleFactory::registerModule(
            "ctt_phi24_resonator", 
            CTTTopologicalResonator::getInstance, 
            CTTTopologicalResonator::getParams
        );
    }
}
}
