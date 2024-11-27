#pragma once

namespace meteor
{
    class DintSampleReader
    {
    private:
        bool iserror = false;
        std::vector<int8_t> buffer1, buffer2;

        void read_more()
        {
            buffer1.resize(buffer1.size() + 8192);
            iserror = iserror ||
                !input_function(&buffer1[buffer1.size() - 8192], 8192);

            buffer2.resize(buffer2.size() + 8192);
            memcpy(&buffer2[buffer2.size() - 8192], &buffer1[buffer1.size() - 8192], 8192);
            rotate_soft(&buffer2[buffer2.size() - 8192], 8192, PHASE_90, false);
        }

    public:
        std::function<int(int8_t*, size_t)> input_function;

        int read1(int8_t* buf, size_t len)
        {
            while (buffer1.size() < len && !iserror)
                read_more();
            if (iserror)
                return 0;
            memcpy(buf, buffer1.data(), len);
            buffer1.erase(buffer1.begin(), buffer1.begin() + len);
            return len;
        }

        int read2(int8_t* buf, size_t len)
        {
            while (buffer2.size() < len && !iserror)
                read_more();
            if (iserror)
                return 0;
            memcpy(buf, buffer2.data(), len);
            buffer2.erase(buffer2.begin(), buffer2.begin() + len);
            return len;
        }
    };
}