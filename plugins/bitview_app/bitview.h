#pragma once

#include "handlers/handler.h"
#include "imgui/imgui.h"
#include <vector>

#include "bit_container.h"

#include "utils/task_queue.h"

namespace satdump
{
    class BitViewTool;

    class BitViewHandler : public handlers::Handler
    {
    protected:
        bool is_busy = false;
        bool reset_view = true;

        void drawMenu();
        void drawContents(ImVec2 win_size);
        void drawMenuBar();
        void drawContextMenu();

    private:
        bool custom_bit_depth = false;
        std::string frame_width_exp;

        std::shared_ptr<BitContainer> bc;

    private:
        float process_progress = 0;
        TaskQueue process_task;

        std::vector<std::shared_ptr<BitViewTool>> all_tools;

    public:
        BitViewHandler(std::shared_ptr<BitContainer> c);
        ~BitViewHandler();

    public:
        std::string getID() { return "bitview_handler"; }
        std::string getName() { return bc->getName(); }
    };
}; // namespace satdump