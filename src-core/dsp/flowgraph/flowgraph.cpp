#include "flowgraph.h"
#include "core/exception.h"
#include "dsp/flowgraph/missing_node.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include <limits>
#include <mutex>

namespace satdump
{
    namespace ndsp
    {
        namespace flowgraph
        {
            int Flowgraph::getNewNodeID()
            {
                for (int i = 0; i < std::numeric_limits<int>::max(); i++)
                {
                    bool already_contained = false;
                    for (auto &n : nodes)
                        if (n->id == i)
                            already_contained = true;
                    if (already_contained)
                        continue;
                    return i;
                }

                throw satdump_exception("No valid ID found for new node ID!");
            }

            int Flowgraph::getNewNodeIOID(std::vector<Node::InOut> *ptr)
            {
                for (int i = 0; i < std::numeric_limits<int>::max(); i++)
                {
                    bool already_contained = false;
                    for (auto &n : nodes) // Check all nodes currently in flowgraph
                        for (auto &io : n->node_io)
                            if (io.id == i)
                                already_contained = true;
                    if (ptr != nullptr)
                        for (auto &io : *ptr) // Check in node being added if required (on creation!)
                            if (io.id == i)
                                already_contained = true;
                    if (already_contained)
                        continue;
                    return i;
                }

                throw satdump_exception("No valid ID found for new node IO ID!");
            }

            int Flowgraph::getNewLinkID()
            {
                for (int i = 0; i < std::numeric_limits<int>::max(); i++)
                {
                    bool already_contained = false;
                    for (auto &l : links) // Check all links
                        if (l.id == i)
                            already_contained = true;
                    if (already_contained)
                        continue;
                    return i;
                }

                throw satdump_exception("No valid ID found for new link ID!");
            }

            void Flowgraph::updateVars()
            {
                std::lock_guard<std::mutex> lg(flow_mtx);

                // For each node, check variables that have an expression set, parse it & set them.
                for (auto &n : nodes)
                {
                    for (auto &v : n->vars)
                    {
                        try
                        {
                            mu::Parser equParser;
                            equParser.SetExpr(v.second);

                            for (auto &var : variables)
                                equParser.DefineConst(var.first, var.second);

                            int nout = 0;
                            double *out = equParser.Eval(nout);

                            nlohmann::json sv;
                            sv[v.first] = *out;

                            if (nout == 1)
                                n->internal->setP(sv);
                            else
                                logger->error("Error parsing expression for %s!", v.first.c_str());
                        }
                        catch (mu::ParserError &e)
                        {
                            logger->error("Error parsing expression for %s (%s)!", v.first.c_str(), e.GetMsg().c_str());
                        }
                    }
                }
            }

            std::shared_ptr<Flowgraph::Node> Flowgraph::addNode(std::string id, std::shared_ptr<NodeInternal> i)
            {
                auto ptr = std::make_shared<Node>(this, id, i);
                nodes.push_back(ptr);
                return ptr;
            }

            nlohmann::json Flowgraph::getJSON()
            {
                std::lock_guard<std::mutex> lg(flow_mtx);

                nlohmann::json j;

                for (auto &n : nodes)
                    j["nodes"][n->id] = n->getJSON();
                j["links"] = links;
                j["vars"] = variables;

                return j;
            }

            void Flowgraph::setJSON(nlohmann::json j)
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

                            try
                            {
                                int ni = 0, no = 0;
                                for (auto &io : n.value()["io"])
                                    if (io["is_out"].get<bool>())
                                        no++;
                                    else
                                        ni++;

                                auto i = std::make_shared<NodeMissing>(this, ni, no);
                                auto nn = std::make_shared<Node>(this, n.value(), i);
                                nodes.push_back(nn);
                            }
                            catch (std::exception &e)
                            {
                                logger->error("Error adding missing node with ID : " + n.value()["int_id"].get<std::string>() + ", Error : %s", e.what());
                            }
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
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump