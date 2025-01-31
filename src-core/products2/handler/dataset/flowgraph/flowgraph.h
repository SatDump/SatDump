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
                : id(j["id"]), internal_id(j["int_id"]), title(i->title), node_io(j["io"]), internal(i)
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
        void run();
    };
}