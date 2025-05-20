#pragma once

#include "handlers/handler.h"
#include "imgui/imgui.h"
#include <cstdint>

namespace satdump
{
    struct Station
    {
    };

    class ObjectTracker
    {
    public:
        virtual void getHorPos(Station station, double time); // AzElRange
    };

    class WipTrackingHandler : public handlers::Handler
    {
    protected:
        void drawMenu();
        void drawContents(ImVec2 win_size);
        void drawMenuBar();

    public:
        struct ObjectPosition
        {
        };

    public:
        WipTrackingHandler();
        ~WipTrackingHandler();

    public:
        std::string getID() { return "wiptracking_handler"; }
        std::string getName() { return "WIP Tracking"; }
    };
}; // namespace satdump
