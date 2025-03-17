#include "node_int.h"
#include "flowgraph.h"

namespace satdump
{
    namespace ndsp
    {
        NodeInternal::NodeInternal(const Flowgraph *f, std::shared_ptr<ndsp::Block> b)
            : f((Flowgraph *)f), blk(b)
        {
            j = blk->get_cfg();
            setupJSONParams();
        }

        void NodeInternal::render()
        {
            // widgets::JSONTreeEditor(v, "##testid", false);
            /*for (auto &c : v.items())
            {
                std::string name = c.key();
                std::string id = name + "##currentblock";
                if (c.value().is_number())
                {
                    double l = c.value();
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputDouble(id.c_str(), &l);
                    c.value() = l;
                }
                else if (c.value().is_string())
                {
                    std::string l = c.value();
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputText(id.c_str(), &l);
                    c.value() = l;
                }
            }*/

            for (auto &c : v)
            {
                ImGui::SetNextItemWidth(120);
                ImGui::InputText(c.first.c_str(), &c.second);
            }
        }

        nlohmann::json NodeInternal::getP()
        {
            return v;
        }

        void NodeInternal::setP(nlohmann::json p)
        {
            for (auto &c : p.items())
                if (v.count(c.key()))
                    v[c.key()] = c.value();

            // v = p;
            //  blk->setParams(p);
            //  v = blk->getParams();
        }

        void NodeInternal::getFinalJSON()
        {
            for (auto &c : j.items())
            {
                if (v.count(c.key()))
                {
                    if (c.value().is_number_integer())
                        c.value() = f->var_manager_test.getIntVariable(v[c.key()]);
                    else if (c.value().is_number())
                        c.value() = f->var_manager_test.getDoubleVariable(v[c.key()]);
                    else if (c.value().is_string())
                        c.value() = v[c.key()];
                }
            }
        }

        void NodeInternal::applyP()
        {
            getFinalJSON();

            printf("\n%s\n", j.dump(4).c_str());

            blk->set_cfg(j);
        }
    }
}