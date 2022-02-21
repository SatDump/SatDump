#include <cstdint>
#include <vector>

namespace noaa
{
    class NOAADeframer
    {
    private:
        unsigned int d_state;
        unsigned int d_bit_count;
        unsigned int d_word_count;
        unsigned long long d_shifter; // 60 bit sync word
        unsigned short d_word;        // 10 bit HRPT word
        int d_thresold;

        void enter_idle();
        void enter_synced();

        bool inverted;

    public:
        NOAADeframer(int thresold);
        std::vector<uint16_t> work(int8_t *input, int length);
    };
} // namespace noaa