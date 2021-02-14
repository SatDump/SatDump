#include <cstdint>
#include <vector>

class NOAADeframer
{
private:
    unsigned int d_state;
    bool d_mid_bit;
    unsigned char d_last_bit;
    unsigned int d_bit_count;
    unsigned int d_word_count;
    unsigned long long d_shifter; // 60 bit sync word
    unsigned short d_word;        // 10 bit HRPT word

    void enter_idle();
    void enter_synced();

public:
    NOAADeframer();
    std::vector<uint16_t> work(uint8_t* input, int length);
};