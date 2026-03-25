#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/path/splitter.h"
#include "flowgraph.h"
#include "logger.h"
#include <chrono>
#include <cstdint>
#include <thread>

namespace satdump
{
    namespace ndsp
    {
        namespace flowgraph
        {
            void Flowgraph::run()
            {
                is_running = true;

                try
                {
                    // Update variables
                    updateVars();

                    // Hold all input IDs that will be connected
                    std::vector<int> all_connected_inputs;

                    // Holds stuff such as splitters when one output goes to more than
                    // one input
                    std::vector<std::shared_ptr<ndsp::Block>> additional_blocks;

                    // Iterate through all nodes
                    for (auto &n : nodes)
                    {
                        if (n->disabled)
                            continue;

                        if (n->internal->is_errored())
                            throw satdump_exception("A node has an error. Abort.");

                        // UI Stuff, safety
                        n->show_vars_win = false;

                        auto &blk = n->internal->blk;

                        // Iterate through outputs
                        for (int o = 0; o < blk->get_outputs().size(); o++)
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

                            // Iterate through links, to asign outputs to applicable inputs
                            struct InputH
                            {
                                std::shared_ptr<ndsp::Block> blk;
                                int idx;
                            };
                            std::vector<InputH> inputs_to_feed;
                            for (auto &l : links)
                            {
                                if (l.start == o_id)
                                {
                                    // Iterate through nodes to find valid inputs
                                    for (auto &n2 : nodes)
                                    {
                                        if (n2->disabled)
                                            continue;

                                        for (int b = 0, b2 = 0; b < n2->node_io.size(); b++)
                                        {
                                            if (!n2->node_io[b].is_out)
                                            {
                                                if (n2->node_io[b].id == l.end)
                                                {
                                                    inputs_to_feed.push_back({n2->internal->blk, b2});
                                                    // logger->trace("Assigned to : " + n2->internal->blk->d_id);
                                                    all_connected_inputs.push_back(l.end);
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
                                        if (n2->disabled)
                                            continue;

                                        for (int b = 0, b2 = 0; b < n2->node_io.size(); b++)
                                        {
                                            if (!n2->node_io[b].is_out)
                                            {
                                                if (n2->node_io[b].id == l.start)
                                                {
                                                    inputs_to_feed.push_back({n2->internal->blk, b2});
                                                    // logger->trace("Assigned to : " + n2->internal->blk->d_id);
                                                    all_connected_inputs.push_back(l.start);
                                                }

                                                b2++;
                                            }
                                        }
                                    }
                                }
                            }

                            // If needed, add splitter
                            if (inputs_to_feed.size() == 1)
                            {
                                inputs_to_feed[0].blk->link(blk.get(), o, inputs_to_feed[0].idx, 16 /*TODOREWORK*/);
                            }
                            else if (inputs_to_feed.size() > 1)
                            {
                                // logger->error("More than one to connect! Adding splitter");

                                std::shared_ptr<ndsp::Block> ptr;
                                if (blk->get_output(o, 0).type == ndsp::BlockIOType::DSP_SAMPLE_TYPE_CF32)
                                    ptr = std::make_shared<ndsp::SplitterBlock<complex_t>>();
                                else if (blk->get_output(o, 0).type == ndsp::BlockIOType::DSP_SAMPLE_TYPE_F32)
                                    ptr = std::make_shared<ndsp::SplitterBlock<float>>();
                                else if (blk->get_output(o, 0).type == ndsp::BlockIOType::DSP_SAMPLE_TYPE_S16)
                                    ptr = std::make_shared<ndsp::SplitterBlock<int16_t>>();
                                else if (blk->get_output(o, 0).type == ndsp::BlockIOType::DSP_SAMPLE_TYPE_S8)
                                    ptr = std::make_shared<ndsp::SplitterBlock<int8_t>>();
                                else if (blk->get_output(o, 0).type == ndsp::BlockIOType::DSP_SAMPLE_TYPE_U8)
                                    ptr = std::make_shared<ndsp::SplitterBlock<uint8_t>>();
                                else
                                    throw satdump_exception("Unsupported splitter block IO type");

                                nlohmann::json p;
                                p["noutputs"] = inputs_to_feed.size();
                                ptr->set_cfg(p);

                                ptr->link(blk.get(), o, 0, 16 /*TODOREWORK*/);
                                for (int v = 0; v < inputs_to_feed.size(); v++)
                                    inputs_to_feed[v].blk->link(ptr.get(), v, inputs_to_feed[v].idx, 16 /*TODOREWORK*/);

                                additional_blocks.push_back(ptr);
                            }
                            else if (inputs_to_feed.size() == 0)
                            {
                                throw satdump_exception("Block has unconnected output!");
                            }
                        }
                    }

                    // Iterate through all nodes, check all inputs are connected
                    for (auto &n : nodes)
                    {
                        if (n->disabled)
                            continue;

                        for (auto &i : n->node_io)
                        {
                            if (i.is_out)
                                continue;

                            bool missing = true;
                            for (auto &c : all_connected_inputs)
                                if (i.id == c)
                                    missing = false;

                            if (missing)
                                throw satdump_exception("Block (" + n->title + ") has unconnected input!");
                        }
                    }

                    // Start them all
                    for (auto &n : nodes)
                        if (!n->disabled)
                            n->internal->blk->start();
                    for (auto &b : additional_blocks)
                        b->start();
                    for (auto &n : nodes)
                        if (!n->disabled)
                            n->internal->upd_state();

                    // And then wait for them to exit
                    for (auto &n : nodes)
                        if (!n->disabled && !n->internal->blk->is_async())
                            n->internal->blk->stop();
                    for (auto &b : additional_blocks)
                        b->stop();

                    // TODOREWORK investigate this
                    std::this_thread::sleep_for(std::chrono::seconds(2));

                    // Restop them all, including async ones
                    for (auto &n : nodes)
                        if (!n->disabled)
                            n->internal->blk->stop();

                    // Re-update UI
                    for (auto &n : nodes)
                        if (!n->disabled)
                            n->internal->upd_state();
                }
                catch (std::exception &e)
                {
                    logger->error("Error running flowgraph : %s", e.what());
                }

                is_running = false;
            }

            void Flowgraph::stop()
            {
                try
                {
                    std::vector<std::thread> all_th;

                    // Iterate through all nodes
                    for (auto &n : nodes)
                    {
                        if (n->disabled)
                            continue;

                        // Stop only those that are sources
                        if (n->internal->blk->is_async())
                        {
                            auto v = [&]
                            {
                                logger->trace("Stopping source " + n->internal->blk->d_id);
                                n->internal->blk->stop(true);
                                logger->trace("Stopped source " + n->internal->blk->d_id);
                            };
                            all_th.push_back(std::thread(v));
                        }
                    }

                    // Wait
                    for (auto &v : all_th)
                        if (v.joinable())
                            v.join();
                }
                catch (std::exception &e)
                {
                    logger->error("Error running flowgraph : %s", e.what());
                }
            }
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump