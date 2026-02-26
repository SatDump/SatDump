#include "pipeline/module.h"

namespace satdump {
namespace pipeline {

    class CTTTopologicalResonator : public ProcessingModule {
    public:
        CTTTopologicalResonator(std::string id, nlohmann::json config) : ProcessingModule(id, id, config) {}
        
        // Satisfying the interface requirements
        void drawUI(bool window) override {}
        void process() override {}
        
        // SatDump v2.0 requirements
        static std::string getID() { return "ctt_phi24_resonator"; }
        static nlohmann::json getParams() { return nlohmann::json::array(); }
        
        // This was the missing link!
        std::string getIDM() override { return "ctt_phi24_resonator"; }
        
        static std::shared_ptr<ProcessingModule> getInstance(std::string id, std::string output_file_hint, nlohmann::json parameters) {
            return std::make_shared<CTTTopologicalResonator>(id, parameters);
        }
    };
}
}
