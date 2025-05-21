#pragma once

#include "handlers/handler.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <thread>
#include <vector>

#include "imgui/dialogs/widget.h"

#include "bit_container.h"

#include "libs/ctpl/ctpl_stl.h"

#include "tool.h"

namespace satdump
{
    class BitViewHandler : public handlers::Handler
    {
    protected:
        bool is_busy = false;

        void drawMenu();
        void drawContents(ImVec2 win_size);
        void drawMenuBar();

    private:
        FileSelectWidget select_bitfile_dialog = FileSelectWidget("File", "Select File", false, true);

        std::shared_ptr<BitContainer> current_bit_container;

    private:
        float process_progress = 0;
        ctpl::thread_pool process_threadp = ctpl::thread_pool(4);

        std::vector<std::shared_ptr<BitViewTool>> all_tools;

    public:
        BitViewHandler();
        BitViewHandler(std::shared_ptr<BitContainer> c);
        ~BitViewHandler();

    public:
        std::string getID() { return "bitview_handler"; }
        std::string getName()
        {
            if (current_bit_container)
                return current_bit_container->getName();
            else
                return "BitView TODOREWORK";
        }
    };
}; // namespace satdump