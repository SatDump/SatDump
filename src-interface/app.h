#pragma once

#include "dll_export.h"
#include <map>
#include <string>
#include <functional>
#include <memory>

namespace satdump
{
    /*
    Some functionalities need to take over the entire window
    */
    class Application
    {
    protected:
        const std::string app_id;
        virtual void drawUI();

    public:
        Application(std::string id);
        ~Application();

        void drawWindow();
        void draw();

    public:
        static std::string getID();
        static std::shared_ptr<Application> getInstance();
    };

    SATDUMP_DLL extern std::map<std::string, std::function<std::shared_ptr<Application>()>> application_registry;

    // Event where applications are registered, so plugins can load theirs
    struct RegisterApplicationsEvent
    {
        std::map<std::string, std::function<std::shared_ptr<Application>()>> &application_registry;
    };

    void registerApplications();
};