#pragma once

/**
 * @file flowgraph.h
 * @brief DSP Flowgraph & Node implementation
 */

#include "dsp/block.h"
#include "nlohmann/json.hpp"
#include "node_int.h"
#include <mutex>
#include <string>
#include <vector>

namespace satdump
{
    namespace ndsp
    {
        namespace flowgraph
        {
            /**
             * @brief Class to hold a DSP flowgraph.
             *
             * A DSP flowgraph is meant to allow creating chains of DSP
             * blocks in an user-friendly way, that's more visually appealing
             * than code :-)
             * Perhaps in the future this will also be useful in CLI, but we'll
             * see. Initially this was only meant to be for personal debugging
             * use!
             *
             * See Node / NodeInternal documentation for more info.
             */
            class Flowgraph
            {
            public:
                /**
                 * @brief Block or Node registry entry.
                 * @param menuname Human-readable name, with an option to
                 * organize by category by providing a string such as
                 * Filter/LPF/RRC Filter, which will be automatically
                 * divided into sub-menus.
                 * @param func NodeInternal creation function
                 */
                struct NodeInternalReg
                {
                    std::string menuname;
                    std::function<std::shared_ptr<NodeInternal>(const Flowgraph *f)> func;
                };

                //! @brief Node registry
                std::map<std::string, NodeInternalReg> node_internal_registry;

            public:
                /**
                 * @brief Flowgraph node class, which holds information
                 * and various configuation for a node.
                 *
                 * Overall the concept is :
                 * - A NodeInternal, virtual class that holds the actual
                 * DSP block and potential custom rendering is held by this class
                 * - IO is updated on request (when NodeInternal->render() returns
                 * true) based on NodeInternal->blk IO info
                 * - Nodes can be disabled, to be ignored when the flowgraph runs
                 * - Upon creation, the node position must be set at least once
                 *
                 * See NodeInternal for further info.
                 */
                class Node
                {
                    friend class Flowgraph;

                private:
                    //! @brief Flowgraph the node belongs to
                    Flowgraph *_f;

                private:
                    const int id;
                    const std::string title;
                    const std::string internal_id;
                    const std::shared_ptr<NodeInternal> internal;

                private:
                    bool pos_was_set = false;
                    float pos_x = 0;
                    float pos_y = 0;

                    //! @brief If false, disables this node
                    bool disabled = false;

                private:
                    //! @brief If true, show the variable expression edit window
                    bool show_vars_win = false;
                    //! @brief List of params that are assigned from expressionbased on flowgraph variables.
                    std::map<std::string, std::string> vars;

                private:
                    /**
                     * @brief Struct holding Input or Output
                     * information mostly for flowgraph rendering
                     * purposes (and links).
                     * @param id unique ID of the IO
                     * @param name readable name/ID of this IO to display
                     * @param is_out true if this is an output
                     * @param type sample/data type this IO sends/receives
                     */
                    struct InOut
                    {
                        int id;
                        std::string name;
                        bool is_out;

                        BlockIOType type;

                        NLOHMANN_DEFINE_TYPE_INTRUSIVE(InOut, id, name, is_out);
                    };

                    //! @brief Node IO for rendering purposes
                    std::vector<InOut> node_io;

                private:
                    /**
                     * @brief Update IR based on internal block. Must be
                     * called on any config / param change that updates
                     * the block's actual IO to sync.
                     */
                    void updateIO()
                    {
                        node_io.clear();
                        for (auto &io : internal->blk->get_inputs())
                            node_io.push_back({_f->getNewNodeIOID(&node_io), io.name, false, io.type});
                        for (auto &io : internal->blk->get_outputs())
                            node_io.push_back({_f->getNewNodeIOID(&node_io), io.name, true, io.type});
                    }

                public:
                    /**
                     * @brief Create a new node (from scratch)
                     * @param f flowgraph this node will belong to
                     * @param id string ID of the node (to display)
                     * @param i internal node class holding the DSP block
                     */
                    Node(Flowgraph *f, std::string id, std::shared_ptr<NodeInternal> i) : _f(f), id(f->getNewNodeID()), title(i->blk->d_id), internal_id(id), internal(i) { updateIO(); }

                    /**
                     * @brief Create a new node with existing JSON config
                     * @param f flowgraph this node will belong to
                     * @param j serialized node JSON to utilize
                     * @param i internal node class holding the DSP block
                     */
                    Node(Flowgraph *f, nlohmann::json j, std::shared_ptr<NodeInternal> i)
                        : _f(f), id(j["id"]), title(i->blk->d_id), internal_id(j["int_id"]), internal(i), node_io(j["io"].get<std::vector<InOut>>())
                    {
                        if (j.contains("int_cfg"))
                            internal->setP(j["int_cfg"]);

                        pos_x = j.contains("pos_x") ? j["pos_x"].get<float>() : 0;
                        pos_y = j.contains("pos_y") ? j["pos_y"].get<float>() : 0;

                        if (j.contains("disabled"))
                            disabled = j["disabled"];

                        if (j.contains("vars"))
                            vars = j["vars"];
                    }

                    /**
                     * @brief Serialize node to JSON
                     * @return JSON of node
                     */
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
                        j["vars"] = vars;
                        return j;
                    }
                };

                /**
                 * @brief Helper structure to hold a flowgraph link
                 * @param id ID of the link
                 * @param start node IO ID this link starts at
                 * @param stop node IO ID this link stops at
                 */
                struct Link
                {
                    int id;
                    int start;
                    int end;

                    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Link, id, start, end);
                };

            private:
                /**
                 * @brief General flowgraph mutex
                 */
                std::mutex flow_mtx;

                /**
                 * @brief Contains all nodes
                 */
                std::vector<std::shared_ptr<Node>> nodes;

                /**
                 * @brief Contains all links
                 */
                std::vector<Link> links;

            private:
                /**
                 * @brief Get a new unique Node ID
                 * @return integer ID for the new node
                 */
                int getNewNodeID();

                /**
                 * @brief Get a new unique Node IO ID
                 * @param ptr node IO list, optionally provided if
                 * the node is being created and not yet in flowgraph!
                 * @return integer ID for the new node IO
                 */
                int getNewNodeIOID(std::vector<Node::InOut> *ptr = nullptr);

                /**
                 * @brief Get a new unique Link ID
                 * @return integer ID for the new link
                 */
                int getNewLinkID();

                /**
                 * @brief Render right-click block list menu.
                 * @param opt Node list to draw (as it can be recursive!)
                 * @param cats categories of block (eg, Filters/LPF)
                 * @param pos position of category (at most cats.size() - 1)
                 * at current recursive depth
                 */
                void renderAddMenu(std::pair<const std::string, NodeInternalReg> &opt, std::vector<std::string> cats, int pos);

            private:
                /**
                 * @brief Helper struct to build a TreeNode menu for node lists
                 * @param cats sub-categories
                 * @param sub the actual nodes
                 */
                struct CatT
                {
                    std::map<std::string, CatT> cats;
                    std::map<std::string, NodeInternalReg> sub;
                };

                /**
                 * @brief Render a CatT and sub-elements as a TreeNode
                 * @param cats CatT to start with
                 * @param searching must be true if search is in use
                 */
                void renderCatT(CatT &cats, bool searching);

            public:
                /**
                 * @brief Render block list menu (right sidebar version).
                 * @param search string to filter the list by
                 */
                void renderAddMenuList(std::string search);

            public:
                /**
                 * @brief Whether to enable debug mode or not.
                 * Debug mode shows extra information for
                 * debugging & diagnostic purposes.
                 */
                bool debug_mode = false;

            public:
                /**
                 * @brief Variables (for easier manipulation of block parameters
                 * via mathematical expressions). Should not be modified while running!
                 */
                std::map<std::string, double> variables;

            public:
                /**
                 * @brief Add a new node in the flowgraph.
                 * @param id string ID to be displayed in the title (usually
                 * block type, but can be anything)
                 * @param i internal node (containing the actual guts of the Node)
                 * @return created node to allow post-creating manipulation on the fly
                 */
                std::shared_ptr<Node> addNode(std::string id, std::shared_ptr<NodeInternal> i);

            public:
                /**
                 * @brief Render the actual flowgraph with ImNodes.
                 * This can be wrapped in a context for zoom if necessary
                 */
                void render();

                /**
                 * @brief Render floating node configuation (and maybe
                 * later other?) windows. Not recommended to wrap in
                 * another context
                 */
                void renderWindows();

            public:
                /**
                 * @brief Update block configs based on variables,
                 * if they have changed.
                 */
                void updateVars();

            public:
                /**
                 * @brief Serialize flowgraph to JSON
                 * @return JSON representation of flowgraph
                 */
                nlohmann::json getJSON();

                /**
                 * @brief De-serialize flowgraph from JSON
                 * @param j JSON repesentation of flowgraph
                 */
                void setJSON(nlohmann::json j);

            private:
                bool is_running = false;

            public:
                /**
                 * @brief Start the flowgraph (or rather, attempt to).
                 * This function will block until the flowgraph either
                 * ends of is (asynchronously) stopped.
                 */
                void run();

                /**
                 * @brief Stops the flowgraph, if started
                 */
                void stop();

                /**
                 * @brief Check if the flowgraph is running
                 * @return true if running
                 */
                bool isRunning() { return is_running; }
            };
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump