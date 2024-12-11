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
        bool d_date_only;
        void handle_input(double input_time);
    public:
        DateTimePicker(std::string id, double input_time, bool date_only = false);
        ~DateTimePicker();
        double get();
        void set(double input_time);
        void draw();
    };
} // namespace widgets