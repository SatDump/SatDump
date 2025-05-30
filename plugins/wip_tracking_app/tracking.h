#pragma once

#include "handlers/handler.h"
#include "imgui/imgui.h"

#include "libs/predict/predict.h"

#include "testgl.h"

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

        OpenGLScene *scene = nullptr;

    public:
        predict_orbital_elements_t *satellite_object = nullptr;
        predict_position satellite_orbit;

    public:
        WipTrackingHandler();
        ~WipTrackingHandler();

    public:
        std::string getID() { return "wiptracking_handler"; }
        std::string getName() { return "WIP Tracking"; }
    };
}; // namespace satdump
