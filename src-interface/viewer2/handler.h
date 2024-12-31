#pragma once

#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <functional>
#include "tree.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace viewer
    {
        class Handler
        { // TODOREWORK PRIVATE DELETE STUFF
        protected:
            TreeDrawerToClean tree_local;
            std::vector<std::shared_ptr<Handler>> subhandlers;
            std::mutex subhandlers_mtx;
            std::vector<std::shared_ptr<Handler>> subhandlers_marked_for_del;

        public:
            virtual void init() = 0;
            virtual void drawMenu() = 0;
            virtual void drawContents(ImVec2 win_size) = 0;

            virtual void drawTreeMenu(std::shared_ptr<Handler> &h);

            virtual std::string getName() { return "!!!Invalid!!!"; }
            std::string getTreeID() { return getName() + "##" + std::to_string((size_t)this); }

            bool hasSubhandlers();

            void addSubHandler(std::shared_ptr<Handler> handler);

            void delSubHandler(std::shared_ptr<Handler> handler);

            static std::string getID();
            static std::shared_ptr<Handler> getInstance();
        };

        inline ImGuiTreeNodeFlags nodeFlags(std::shared_ptr<Handler> &h, bool sec)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
            if (!h->hasSubhandlers())
                flags |= ImGuiTreeNodeFlags_Leaf;
            if (sec)
                flags |= ImGuiTreeNodeFlags_Selected;
            return flags;
        }

        extern std::map<std::string, std::function<std::shared_ptr<Handler>()>> handlers_registry; // TODOREWORK?
        void registerHandlers();
    }
}