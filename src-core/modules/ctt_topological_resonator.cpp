#include <complex>
#include <vector>
#include <iostream>
#include <cmath>
#include <memory>
#include "pipeline/module.h"
#include "nlohmann/json.hpp"

// Forward declaration of factory
namespace satdump { namespace pipeline {
    class ProcessingModuleFactory {
    public:
        static void registerModule(std::string id, 
            std::function<std::shared_ptr<ProcessingModule>(std::string, nlohmann::json)> instantiator, 
            std::function<nlohmann::json()> params);
    };
}}

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
            std::cout << "[CTT] Phi-24 Resonator: Accessing Manifold Ports..." << std::endl;
        }

        void process() override {
            // Accessing the protected port maps directly
            if (m_input_ports.find("in") == m_input_ports.end() || 
                m_output_ports.find("out") == m_output_ports.end()) return;

            auto in_port = m_input_ports["in"];
            auto out_port = m_output_ports["out"];

            if (!in_port || !out_port) return;

            // Cast the raw data manifold to our complex vector
            auto input_ptr = std::dynamic_pointer_cast<std::vector<std::complex<float>>>(in_port->getData());
            auto output_ptr = std::dynamic_pointer_cast<std::vector<std::complex<float>>>(out_port->getData());

            if (!input_ptr || !output_ptr) return;

            size_t size = input_ptr->size();
            output_ptr->resize(size);

            for (size_t i = 0; i < size; ++i) {
                // PLL Logic: Topological phase alignment
                std::complex<float> lo = std::exp(std::complex<float>(0, -phase));
                std::complex<float> mixed = (*input_ptr)[i] * lo;

                float error = std::arg(mixed);
                phase += freq_offset + alpha * error;
                freq_offset += beta * error;

                // Circular phase wrapping
                if (phase > 3.14159265f) phase -= 6.2831853f;
                if (phase < -3.14159265f) phase += 6.2831853f;

                (*output_ptr)[i] = mixed;
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
