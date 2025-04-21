#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"

namespace rotator
{
    enum rotator_status_t
    {
        ROT_ERROR_OK = 0,       // No error
        ROT_ERROR_CMD = 1,      // Command error
        ROT_ERROR_CON = 2,      // Connection error
        ROT_ERROR_INV_ARG = 3,  // Invalid argument
    };

    class RotatorHandler
    {
    public:
        virtual std::string get_id() = 0;
        virtual void set_settings(nlohmann::json settings) = 0;
        virtual nlohmann::json get_settings() = 0;
        virtual rotator_status_t get_pos(float *az, float *el) = 0;
        virtual rotator_status_t set_pos(float az, float el) = 0;
        virtual void render() = 0;
        virtual bool is_connected() = 0;
        virtual void connect() = 0;
        virtual void disconnect() = 0;
    public:
      std::array<float, 2> azLimits = {0, 360.0};
      std::array<float, 2> elLimits = {0, 90.0};
    };

    struct RotatorHandlerOption
    {
        std::string name;
        std::function<std::shared_ptr<RotatorHandler>()> construct;
    };

    struct RequestRotatorHandlerOptionsEvent
    {
        std::vector<RotatorHandlerOption> &opts;
    };

    std::vector<RotatorHandlerOption> getRotatorHandlerOptions();
}
