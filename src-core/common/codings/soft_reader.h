#pragma once

#include "core/module.h"

class SoftSymbolReader
{
private:
    std::istream &data_in;
    std::shared_ptr<dsp::RingBuffer<uint8_t>> &input_fifo;
    ModuleDataType &input_data_type;
    bool convert_from_hard = false;

    int tmp_hard_buff_pos = 0;
    uint8_t *tmp_hard_buff;

    void readMore()
    {
        if (input_data_type == DATA_FILE)
            data_in.read((char *)tmp_hard_buff, 1024);
        else
            input_fifo->read((uint8_t *)tmp_hard_buff, 1024);
        tmp_hard_buff_pos = 0;
    }

public:
    SoftSymbolReader(std::istream &data_in,
                     std::shared_ptr<dsp::RingBuffer<uint8_t>> &input_fifo,
                     ModuleDataType &input_data_type,
                     nlohmann::json &params)
        : data_in(data_in), input_fifo(input_fifo), input_data_type(input_data_type)
    {
        if (params.contains("hard_symbols"))
            convert_from_hard = params["hard_symbols"];

        if (convert_from_hard)
            tmp_hard_buff = new uint8_t[1024];
    }

    ~SoftSymbolReader()
    {
        if (convert_from_hard)
            delete[] tmp_hard_buff;
    }

    inline void readSoftSymbols(int8_t *buffer, int length)
    {
        if (convert_from_hard)
        {
            for (int i = 0; i < length; i++)
            {
                uint8_t bit = (tmp_hard_buff[tmp_hard_buff_pos / 8] >> (7 - (tmp_hard_buff_pos % 8))) & 1;
                buffer[i] = bit ? 70 : -70;
                tmp_hard_buff_pos++;
                if (tmp_hard_buff_pos == 1024 * 8)
                    readMore();
            }
        }
        else
        {
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, length);
            else
                input_fifo->read((uint8_t *)buffer, length);
        }
    }
};
