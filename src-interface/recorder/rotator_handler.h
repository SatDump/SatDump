#pragma once

class RotatorHandler
{
public:
    enum rotator_status_t
    {
        ROT_ERROR_OK = 0,  // No error
        ROT_ERROR_CMD = 1, // Command error
        ROT_ERROR_CON = 2, // Connection error
    };

public:
    virtual rotator_status_t get_pos(float *az, float *el) = 0;
    virtual rotator_status_t set_pos(float az, float el) = 0;
    virtual void render() = 0;
    virtual bool is_connected() = 0;
};