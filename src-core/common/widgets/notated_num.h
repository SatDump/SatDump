#pragma once
#include <string>

namespace widgets {
    template <typename T>
    class NotatedNum
    {
    public:
        NotatedNum(std::string d_id, T input_int, std::string units);
        ~NotatedNum();
        T get();
        void set(T input_int);
        bool draw();
        void parse_input();
        std::string display_val;
    private:
        T val;
        std::string d_id, units, last_display;
    };
}  // namespace widgets