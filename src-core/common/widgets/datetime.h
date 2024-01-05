#pragma once
#include <time.h>
#include <string>

namespace widgets {
    class DateTimePicker
    {
    private:
        struct tm timestamp;
        bool auto_time, show_picker, start_edit;
        int year_holder, month_holder, seconds_decimal;
        std::string d_id;
        void handle_input(double input_time);
    public:
        DateTimePicker(std::string d_id, double input_time);
        ~DateTimePicker();
        double get();
        void set(double input_time);
        void draw();
    };
} // namespace widgets