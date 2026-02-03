#pragma once

#include "nlohmann/json.hpp"
#include <exception>
#include <mutex>
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
                Flowgraph *_f;

                const int id;
                const std::string title;
                const std::string internal_id;
                const std::shared_ptr<NodeInternal> internal;

                bool pos_was_set = false;
                float pos_x = 0;
                float pos_y = 0;

                bool disabled = false;

                struct InOut
                {
                    int id;
                    std::string name;
                    bool is_out;

                    BlockIOType type;

                    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InOut, id, name, is_out);
                };

                std::vector<InOut> node_io;

                void updateIO()
                {
                    node_io.clear();
                    for (auto &io : internal->blk->get_inputs())
                        node_io.push_back({_f->getNewNodeIOID(&node_io), io.name, false, io.type});
                    for (auto &io : internal->blk->get_outputs())
                        node_io.push_back({_f->getNewNodeIOID(&node_io), io.name, true, io.type});
                }

            public:
                Node(Flowgraph *f, std::string id, std::shared_ptr<NodeInternal> i) : _f(f), id(f->getNewNodeID()), internal_id(id), title(i->blk->d_id), internal(i) { updateIO(); }

                Node(Flowgraph *f, nlohmann::json j, std::shared_ptr<NodeInternal> i)
                    : id(j["id"]), internal_id(j["int_id"]), title(i->blk->d_id), node_io(j["io"].get<std::vector<InOut>>()), internal(i)
                {
                    if (j.contains("int_cfg"))
                        internal->setP(j["int_cfg"]);

                    pos_x = j.contains("pos_x") ? j["pos_x"].get<float>() : 0;
                    pos_y = j.contains("pos_y") ? j["pos_y"].get<float>() : 0;

                    if (j.contains("disabled"))
                        disabled = j["disabled"];
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
                    j["disabled"] = disabled;
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
            std::mutex flow_mtx;

            std::vector<std::shared_ptr<Node>> nodes;
            std::vector<Link> links;

            int getNewNodeID();
            int getNewNodeIOID(std::vector<Node::InOut> *ptr = nullptr);
            int getNewLinkID();

            void renderAddMenu(std::pair<const std::string, NodeInternalReg> &opt, std::vector<std::string> cats, int pos);

        public:
            bool debug_mode = false;

        public:
            Flowgraph();
            ~Flowgraph();

            std::map<std::string, double> variables;

            std::shared_ptr<Node> addNode(std::string id, std::shared_ptr<NodeInternal> i)
            {
                auto ptr = std::make_shared<Node>(this, id, i);
                nodes.push_back(ptr);
                return ptr;
            }

            void render();

            nlohmann::json getJSON()
            {
                std::lock_guard<std::mutex> lg(flow_mtx);
                nlohmann::json j;
                for (auto &n : nodes)
                    j["nodes"][n->id] = n->getJSON();
                j["links"] = links;
                j["vars"] = variables;
                return j;
            }

            void setJSON(nlohmann::json j)
            {
                std::lock_guard<std::mutex> lg(flow_mtx);
                if (j.contains("vars"))
                    variables = j["vars"];

                nodes.clear();
                links.clear();

                for (auto &n : j["nodes"].items())
                {
                    if (n.value().contains("int_id"))
                    {
                        if (node_internal_registry.count(n.value()["int_id"]))
                        {
                            try
                            {
                                auto i = node_internal_registry[n.value()["int_id"]].func(this);
                                auto nn = std::make_shared<Node>(this, n.value(), i);
                                nodes.push_back(nn);
                            }
                            catch (std::exception &e)
                            {
                                logger->error("Error adding node with ID : " + n.value()["int_id"].get<std::string>() + ", Error : %s", e.what());
                            }
                        }
                        else
                        {
                            logger->error("Could not find node with ID : " + n.value()["int_id"].get<std::string>());
                        }
                    }
                    else
                    {
                        logger->error("Node is missing int_id!");
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

                // Update node IOs to reflect the proper types on links
                for (auto &n : nodes)
                {
                    size_t ic = 0, oc = 0;
                    for (auto &io : n->node_io)
                    {
                        if (io.is_out)
                        {
                            if (n->internal->blk->get_outputs().size() > oc)
                                io.type = n->internal->blk->get_outputs()[oc].type;
                            oc++;
                        }
                        else
                        {
                            if (n->internal->blk->get_inputs().size() > ic)
                                io.type = n->internal->blk->get_inputs()[ic++].type;
                            ic++;
                        }
                    }
                }
            }

        public:
            bool is_running = false;

            void run();
            void stop();
        };
    } // namespace ndsp
} // namespace satdump