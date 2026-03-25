#pragma once

#include "common/widgets/json_editor.h"
#include "dsp/agc/agc.h"
#include "dsp/block.h"
#include "dsp/flowgraph/node_int.h"
#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace ndsp
    {
        class DummyBlock : public Block
        {
        private:
            bool work() { return true; }

        public:
            DummyBlock(int ni, int no) : Block("dummy")
            {
                inputs.resize(ni, {"in", DSP_SAMPLE_TYPE_CF32});
                inputs.resize(no, {"out", DSP_SAMPLE_TYPE_CF32});
            }

            ~DummyBlock() {}

            nlohmann::ordered_json get_cfg_list() { return nlohmann::ordered_json(); }
            nlohmann::json get_cfg(std::string key) { return nlohmann::json(); }
            cfg_res_t set_cfg(std::string key, nlohmann::json v) { return RES_OK; }
        };

        namespace flowgraph
        {
            class NodeMissing : public NodeInternal
            {
            public:
                NodeMissing(const Flowgraph *f, int ni, int no) : NodeInternal(f, std::make_shared<DummyBlock>(ni, no)) { is_error = true; }

                nlohmann::json params;

                virtual nlohmann::json getP() { return params; }

                virtual void setP(nlohmann::json p) { params = p; }

                virtual bool render()
                {
                    // ImGui::PushItemWidth(200);
                    // widgets::JSONTableEditor(params, "Editor");
                    // ImGui::PopItemWidth();
                    return false;
                }
            };
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump