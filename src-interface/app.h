#pragma once

#include <memory>
#include <string>
#include <vector>

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
        void draw(bool window = true);

    public:
        static std::string getID();
        virtual std::string get_name() = 0;
        static std::shared_ptr<Application> getInstance();
    };

    struct AddGUIApplicationEvent
    {
        std::vector<std::shared_ptr<Application>> &applications;
    };
}; // namespace satdump