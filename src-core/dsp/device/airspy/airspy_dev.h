#pragma once

#include "../dev.h"

#include <libairspy/airspy.h>

namespace satdump
{
    namespace ndsp
    {
        class AirspyDevBlock : public DeviceBlock
        {
        public:
            float p_rate = 1e-4;

        private:
            airspy_device *airspy_dev_obj;

            bool work();

        public:
            AirspyDevBlock();
            ~AirspyDevBlock();

            void init()
            {
                //   rate = p_rate;
            }

            nlohmann::json get_cfg_list()
            {
                nlohmann::json p;
                p["rate"]["type"] = "float";
                p["reference"]["type"] = "float";
                return p;
            }

            nlohmann::json get_cfg()
            {
                nlohmann::json v;
                v["rate"] = p_rate;
                return v;
            }

            void set_cfg(nlohmann::json v)
            {
                setValFromJSONIfExists(p_rate, v["rate"]);
            }

            void drawUI()
            {
            }

            virtual void start();
            virtual void stop(bool stop_now = false);

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    }
}