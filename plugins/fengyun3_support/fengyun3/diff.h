#include <cstdint>

namespace fengyun3
{
    // Differential decoder
    class FengyunDiff
    {
    private:
        unsigned char Xin_1 = 0, Yin_1 = 0, Xin = 0, Yin = 0, Xout = 0, Yout = 0;
        uint8_t v = 0;
        char inBuf = 0;    // Counter used at the beggining
        uint8_t buffer[2]; // Smaller buffer for internal use

        uint8_t mask[4] = {0b00000000, 0b11000000, 0b11110000, 0b11111100};

    public:
        void work(uint8_t *in, int len, uint8_t *out);

        void work2(uint8_t *in1, uint8_t *in2, int len, uint8_t *out);
    };
} // namespace fengyun