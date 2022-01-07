#include <cstdint>

namespace diff
{
    // Differential decoder
    class QPSKDiff
    {
    private:
        unsigned char Xin_1, Yin_1, Xin, Yin, Xout, Yout;
        char inBuf = 0;    // Counter used at the beggining
        uint8_t buffer[2]; // Smaller buffer for internal use
        int oo = 0;
        uint8_t ou;

    public:
        void work(uint8_t *in, int len, uint8_t *out);
        bool swap = true;
    };
} // namespace fengyun