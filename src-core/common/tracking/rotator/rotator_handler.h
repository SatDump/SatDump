#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"

namespace rotator
{
    enum rotator_status_t
    {
        ROT_ERROR_OK = 0,  // No error
        ROT_ERROR_CMD = 1, // Command error
        ROT_ERROR_CON = 2, // Connection error
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
    };
}