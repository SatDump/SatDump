#pragma once

class RotatorHandler
{
public:
    virtual bool get_pos(float *az, float *el) = 0;
    virtual bool set_pos(float az, float el) = 0;
    virtual void render() = 0;
    virtual bool is_connected() = 0;
};