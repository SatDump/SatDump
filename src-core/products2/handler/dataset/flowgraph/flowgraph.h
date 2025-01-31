#pragma once

#include <vector>
#include <string>
#include "nlohmann/json.hpp"

//// TODOREWORK
#include "logger.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"

namespace satdump
{
    // TODOREWORK proper sub-namespace?

    class NodeInternal
    {
    public:
        const std::string title;

        struct InOutConfig
        {
            std::string name;
            std::shared_ptr<void> ptr = nullptr;
        };

        std::vector<InOutConfig> inputs;
        std::vector<InOutConfig> outputs;
        bool has_run;

    public:
        std::function<void(InOutConfig, bool)> add_io_callback;

        void addInputDynamic(InOutConfig cfg)
        {
            inputs.push_back(cfg);
            add_io_callback(cfg, false);
        }

    public:
        void reset()
        {
            has_run = false;
            for (auto &i : inputs)
                i.ptr.reset();
            for (auto &o : outputs)
                o.ptr.reset();
        }

        bool can_run()
        {
            bool v = true;
            for (auto &i : inputs)
                if (!i.ptr)
                    v = false;
            return v;
        }

        virtual void process() = 0;
        virtual void render() = 0;
        virtual nlohmann::json to_json() = 0;
        virtual void from_json(nlohmann::json j) = 0;

    public:
        NodeInternal(std::string title) : title(title) {}
        virtual ~NodeInternal() {}
    };

    class Flowgraph
    {
    public:
        std::map<std::string, std::function<std::shared_ptr<NodeInternal>()>> node_internal_registry;

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
            Node(Flowgraph *f, std::string id, std::shared_ptr<NodeInternal> i)
                : id(f->getNewNodeID()), internal_id(id), title(i->title), internal(i)
            {
                for (auto &io : internal->inputs)
                    node_io.push_back({f->getNewNodeIOID(&node_io), io.name, false});
                for (auto &io : internal->outputs)
                    node_io.push_back({f->getNewNodeIOID(&node_io), io.name, true});
                internal->add_io_callback = [this, f](NodeInternal::InOutConfig io, bool out)
                {
                    node_io.push_back({f->getNewNodeIOID(&node_io), io.name, out});
                };
            }

            Node(Flowgraph *f, nlohmann::json j, std::shared_ptr<NodeInternal> i)
                : id(j["id"]), internal_id(j["int_id"]), title(i->title), node_io(j["io"].get<std::vector<InOut>>()), internal(i)
            {
                internal->from_json(j["int_cfg"]);
                internal->add_io_callback = [this, f](NodeInternal::InOutConfig io, bool out)
                {
                    node_io.push_back({f->getNewNodeIOID(&node_io), io.name, out});
                };

                pos_x = j.contains("pos_x") ? j["pos_x"].get<float>() : 0;
                pos_y = j.contains("pos_y") ? j["pos_y"].get<float>() : 0;
            }

            nlohmann::json getJSON()
            {
                nlohmann::json j;
                j["id"] = id;
                j["io"] = node_io;
                j["int_id"] = internal_id;
                j["int_cfg"] = internal->to_json();
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

    public:
        Flowgraph();
        ~Flowgraph();

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
            return j;
        }

        void setJSON(nlohmann::json j)
        {
            nodes.clear();
            links.clear();

            for (auto &n : j["nodes"].items())
            {
                auto i = node_internal_registry[n.value()["int_id"]]();
                nodes.push_back(std::make_shared<Node>(this, n.value(), i));
            }
            links = j["links"];
        }

    public:
        void run()
        {
            for (auto &n : nodes)
                n->internal->reset();

            try
            {
                bool should_run_again = true;
                int step_number = 0;
                while (should_run_again)
                {
                    should_run_again = false;

                    // Iterate through all nodes
                    for (auto &n : nodes)
                    {
                        // Check if this one can run
                        auto &i = n->internal;
                        if (i->can_run() && !i->has_run)
                        {
                            // Run it.
                            i->process();
                            logger->debug("Step %d Ran : %s", step_number, i->title.c_str());

                            // Iterate through outputs
                            for (int o = 0; o < i->outputs.size(); o++)
                            {
                                // Get output ID
                                int o_id = -1;
                                for (int c = 0, oc = 0; c < n->node_io.size(); c++)
                                {
                                    if (n->node_io[c].is_out)
                                    {
                                        if (oc == o)
                                            o_id = n->node_io[c].id;
                                        oc++;
                                    }
                                }

                                logger->trace("Output ID for %d is %d", o, o_id);

                                // Iterate through links, to asign outputs to applicable inputs
                                for (auto &l : links)
                                {
                                    if (l.start == o_id)
                                    {
                                        // Iterate through nodes to find valid inputs
                                        for (auto &n2 : nodes)
                                        {
                                            for (int b = 0, b2 = 0; b < n2->node_io.size(); b++)
                                            {
                                                if (!n2->node_io[b].is_out)
                                                {
                                                    if (n2->node_io[b].id == l.end)
                                                    {
                                                        n2->internal->inputs[b2] = i->outputs[o];
                                                        logger->trace("Assigned to : " + n2->internal->title);
                                                    }

                                                    b2++;
                                                }
                                            }
                                        }
                                    }

                                    if (l.end == o_id)
                                    {
                                        // Iterate through nodes to find valid inputs
                                        for (auto &n2 : nodes)
                                        {
                                            for (int b = 0, b2 = 0; b < n2->node_io.size(); b++)
                                            {
                                                if (!n2->node_io[b].is_out)
                                                {
                                                    if (n2->node_io[b].id == l.start)
                                                    {
                                                        n2->internal->inputs[b2] = i->outputs[o];
                                                        logger->trace("Assigned to : " + n2->internal->title);
                                                    }

                                                    b2++;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            should_run_again = true;
                        }
                    }

                    step_number++;
                }

                logger->info("Flowgraph took %d steps to run!", step_number);
            }
            catch (std::exception &e)
            {
                logger->error("Error running flowgraph : %s", e.what());
            }

            for (auto &n : nodes)
                n->internal->reset();
        }
    };
}