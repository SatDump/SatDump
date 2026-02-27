#include "node_int.h"
#include "common/widgets/CircularProgressBar.h"
#include "core/style.h"
#include "flowgraph.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace ndsp
    {
        NodeInternal::NodeInternal(const Flowgraph *f, std::shared_ptr<ndsp::Block> b) : f((Flowgraph *)f), blk(b)
        {
            optdisp = std::make_unique<ndsp::OptDisplayerWarper>(blk);
            optdisp->setShowStats(true);
        }

        bool NodeInternal::render()
        {
            ImGui::PushItemWidth(200);
            bool r = optdisp->draw();
            ImGui::PopItemWidth();

            return r;
        }

        nlohmann::json NodeInternal::getP() { return blk->get_cfg(); }

        void NodeInternal::setP(nlohmann::json p)
        {
            blk->set_cfg(p);
            optdisp->update();
        }
    } // namespace ndsp
} // namespace satdump
