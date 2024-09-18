#include <cstring>
#include "huffman.h"
#include "tables.h"

namespace meteor
{
    namespace msumr
    {
        namespace lrpt
        {
            // GetQuantizationTable returns the standard quantization table
            // with the quality factor correction.
            std::array<int64_t, 64> GetQuantizationTable(float qf)
            {
                std::array<int64_t, 64> table;

                if (qf > 20 && qf < 50)
                    qf = 5000.0f / qf;
                else
                    qf = 200.0f - (2.0f * qf);

                for (int x = 0; x < 64; x++)
                {
                    table[x] = int64_t((qf / 100.0f * qTable[x]) + 0.5f);
                    if (table[x] < 1)
                        table[x] = 1;
                }
                return table;
            }

            int64_t getValue(bool *dat, int len)
            {
                int64_t result = 0;
                for (int i = 0; i < len; i++)
                {
                    if (dat[i])
                    {
                        result = result | 0x01 << (len - 1 - i);
                    }
                }
                if (!dat[0])
                {
                    result -= (1 << (len)) - 1;
                }
                return result;
            }

            // FindDC decodes and return the next DC coefficient by
            // applying Huffman to the bool slice received.
            int64_t FindDC(bool *&dat, int &length)
            {
                bool *buf = &dat[0];
                int bufl = length; // := len(*dat)

                //std::cout << "DC_BUFL " << bufl << std::endl;

                for (const dc_t &m : dcCategories)
                {
                    if (bufl < m.klen)
                        continue;

                    //std::cout << "DC_KLEN " <<  m.klen << std::endl;

                    if (std::memcmp(buf, m.code, m.klen * sizeof(bool)) == 0)
                    {
                        // std::cout << "DC_EQUAL" << std::endl;
                        if (bufl < m.klen + m.len)
                            break;

                        dat = &buf[m.klen + m.len];
                        length -= m.klen + m.len;
                        //std::cout << "DC_MINUSLEN " <<  m.klen + m.len << std::endl;
                        if (m.len == 0)
                            return 0;

                        return getValue(&buf[m.klen], m.len);
                    }
                }

                //std::cout << "START " << length << " " <<  std::endl;

                //dat = nullptr;
                length = 0;
                return CFC[0];
            }

            // FindAC decodes and return the AC coefficient by applying Huffman.
            std::vector<int64_t> FindAC(bool *&dat, int &length)
            {
                int bufl = length; // := len(*dat)
                //std::cout << "AC_BUFL = " << bufl << std::endl;
                for (const ac_t &m : acCategories)
                {
                    if (bufl < m.klen)
                        continue;

                    //std::cout << "AC_KLEN " <<  m.klen << std::endl;

                    if (std::memcmp(dat, m.code, m.klen * sizeof(bool)) == 0)
                    {
                        if (m.clen == 0 && m.zlen == 0)
                        {
                            dat = &dat[m.klen];
                            length -= m.klen;
                            // std::cout << "AC_MINUSLEN0 " <<  m.klen  << std::endl;
                            return {EOB[0]};
                        }

                        //vals := make([]int64, m.zlen+1)
                        std::vector<int64_t> vals(m.zlen + 1);
                        if (!(m.zlen == 15 && m.clen == 0))
                        {
                            if (bufl < m.klen + m.clen)
                                break;

                            vals[m.zlen] = getValue(&dat[m.klen], m.clen);
                            // std::cout << "AC_ZLEN " << m.clen << " VAL " << vals[m.zlen] << std::endl;
                        }
                        dat = &dat[m.klen + m.clen];
                        length -= m.klen + m.clen;
                        // std::cout << "AC_MINUSLEN " <<  m.klen + m.clen << std::endl;
                        return vals;
                    }
                }

                //dat = nullptr;
                length = 0;
                return {CFC[0]};
            }

            void convertToArray(bool *soft, uint8_t *buf, int length)
            {
                for (int i = 0; i < length; i++)
                {
                    soft[0 + 8 * i] = buf[i] >> 7 & (0x01 == 0x01);
                    soft[1 + 8 * i] = buf[i] >> 6 & (0x01 == 0x01);
                    soft[2 + 8 * i] = buf[i] >> 5 & (0x01 == 0x01);
                    soft[3 + 8 * i] = buf[i] >> 4 & (0x01 == 0x01);
                    soft[4 + 8 * i] = buf[i] >> 3 & (0x01 == 0x01);
                    soft[5 + 8 * i] = buf[i] >> 2 & (0x01 == 0x01);
                    soft[6 + 8 * i] = buf[i] >> 1 & (0x01 == 0x01);
                    soft[7 + 8 * i] = buf[i] >> 0 & (0x01 == 0x01);
                }
            }
        } // namespace lrpt
    }     // namespace msumr
} // namespace meteor