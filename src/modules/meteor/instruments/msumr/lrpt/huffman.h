#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace meteor
{
    namespace msumr
    {
        namespace lrpt
        {
            // EOB indicates the End Of Block of each MCU.
            static int64_t EOB[] = {-99999};

            // CFC indicates that no match was found inside the Huffman LUT.
            static int64_t CFC[] = {-99998};

            void convertToArray(bool *soft, uint8_t *buf, int length);
            std::array<int64_t, 64> GetQuantizationTable(float qf);
            int64_t FindDC(bool *&dat, int &length);
            std::vector<int64_t> FindAC(bool *&dat, int &length);
        } // namespace lrpt
    }     // namespace msumr
} // namespace meteor