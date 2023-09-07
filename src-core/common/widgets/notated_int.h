#include <string>

namespace widgets {
    class NotatedInt
    {
    public:
        NotatedInt(std::string d_id, int64_t input_int, std::string units);
        ~NotatedInt();
        int get();
        void set(uint64_t input_int);
        void draw();
        void parse_input();
        std::string display_val;
    private:
        int64_t val;
        std::string d_id, units, last_display;
        void render_int();
    };
}  // namespace widgets