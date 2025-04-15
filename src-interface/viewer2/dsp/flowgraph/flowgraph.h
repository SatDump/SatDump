#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <vector>

//// TODOREWORK
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"

#include "dsp/block.h"

#include "common/widgets/json_editor.h"

// #include "variable_manager.h"

#include "node_int.h"

namespace satdump
{
    namespace ndsp
    {
        class Flowgraph
        {
        public:
            struct NodeInternalReg
            {
                std::string menuname;
                std::function<std::shared_ptr<NodeInternal>(const Flowgraph *f)> func;
            };
            std::map<std::string, NodeInternalReg> node_internal_registry;

        public:
            class Node
            {
                friend class Flowgraph;

            private:
                const int id;
                const std::string title;
                const std::string internal_id;
                const std::shared_ptr<NodeInternal> internal;

                bool pos_was_set = false;
                float pos_x = 0;
                float pos_y = 0;

                struct InOut
                {
                    int id;
                    std::string name;
                    bool is_out;

                    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InOut, id, name, is_out);
                };

                std::vector<InOut> node_io;

            public:
                Node(Flowgraph *f, std::string id, std::shared_ptr<NodeInternal> i) : id(f->getNewNodeID()), internal_id(id), title(i->blk->d_id), internal(i)
                {
                    for (auto &io : internal->blk->get_inputs())
                        node_io.push_back({f->getNewNodeIOID(&node_io), io.name, false});
                    for (auto &io : internal->blk->get_outputs())
                        node_io.push_back({f->getNewNodeIOID(&node_io), io.name, true});
                }

                Node(Flowgraph *f, nlohmann::json j, std::shared_ptr<NodeInternal> i)
                    : id(j["id"]), internal_id(j["int_id"]), title(i->blk->d_id), node_io(j["io"].get<std::vector<InOut>>()), internal(i)
                {
                    if (j.contains("int_cfg"))
                        internal->setP(j["int_cfg"]);

                    pos_x = j.contains("pos_x") ? j["pos_x"].get<float>() : 0;
                    pos_y = j.contains("pos_y") ? j["pos_y"].get<float>() : 0;
                }

                nlohmann::json getJSON()
                {
                    nlohmann::json j;
                    j["id"] = id;
                    j["io"] = node_io;
                    j["int_id"] = internal_id;
                    j["int_cfg"] = internal->getP();
                    j["pos_x"] = pos_x;
                    j["pos_y"] = pos_y;
                    return j;
                }
            };

            struct Link
            {
                int id;
                int start;
                int end;

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(Link, id, start, end);
            };

        private:
            std::vector<std::shared_ptr<Node>> nodes;
            std::vector<Link> links;

            int getNewNodeID();
            int getNewNodeIOID(std::vector<Node::InOut> *ptr = nullptr);
            int getNewLinkID();

            void renderAddMenu(std::pair<const std::string, NodeInternalReg> &opt, std::vector<std::string> cats, int pos);

        public:
            Flowgraph();
            ~Flowgraph();

            //            LuaVariableManager var_manager_test;

            std::shared_ptr<Node> addNode(std::string id, std::shared_ptr<NodeInternal> i)
            {
                auto ptr = std::make_shared<Node>(this, id, i);
                nodes.push_back(ptr);
                return ptr;
            }

            void render();

            nlohmann::json getJSON()
            {
                nlohmann::json j;
                for (auto &n : nodes)
                    j["nodes"][n->id] = n->getJSON();
                j["links"] = links;
                //                j["vars"] = var_manager_test.variables;
                return j;
            }

            void setJSON(nlohmann::json j)
            {
                //                var_manager_test.variables = j["vars"];

                nodes.clear();
                links.clear();

                for (auto &n : j["nodes"].items())
                {
                    if (node_internal_registry.count(n.value()["int_id"]))
                    {
                        auto i = node_internal_registry[n.value()["int_id"]].func(this);
                        nodes.push_back(std::make_shared<Node>(this, n.value(), i));
                    }
                    else
                    {
                        logger->error("Could not find node with ID : " + n.value()["int_id"].get<std::string>());
                    }
                }

                // Links need to be filtered in case some blocks are missing!
                std::vector<Link> tmp_links = j["links"];

                for (auto &link : tmp_links)
                {
                    bool got_in = false, got_ou = false;
                    for (auto &n : nodes)
                    {
                        for (auto &io : n->node_io)
                        {
                            if (io.id == link.start)
                                got_in = true;
                            if (io.id == link.end)
                                got_ou = true;
                        }
                    }

                    if (got_in && got_ou)
                        links.push_back(link);
                }
            }

        public:
            bool is_running = false;

            void run();
            void stop();
        };
    } // namespace ndsp
} // namespace satdump
