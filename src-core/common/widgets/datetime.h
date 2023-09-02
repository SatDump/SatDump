#pragma once
#include <time.h>
#include <string>
#include "imgui/imgui.h"

namespace widgets {
    class DateTimePicker
    {
    private:
        struct tm *timestamp;
        std::string seconds_decimal;
        bool auto_time;
        int year_holder, month_holder;
        float item_spacing_y, item_inner_spacing_y, frame_padding_y;
        void handle_input(float input_time);
    public:
        DateTimePicker(float input_time);
        ~DateTimePicker();
        float get();
        void set(float input_time);
        void draw();
    };
} // namespace widgets