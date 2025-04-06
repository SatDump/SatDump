#pragma once

#include <cstdint>
#include "nlohmann/json.hpp"

namespace dbf_file
{
    // #pragma pack(push)
    // #pragma pack(1)
    struct DbfHeader
    {
        uint8_t m_iType;
        char m_arcLastUpdate[3];

        uint32_t m_uNumRecords;

        uint16_t m_uFirstRecordOffset;
        uint16_t m_uRecordSize;

        char m_uReserved[15];
        uint8_t m_fFlags;
        uint8_t m_uCodePageMark;

        char m_uReserved2[2];
    };
    // #pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
    struct DbfRecord
    {
        char m_archName[11];
        char chFieldType;

        uint32_t m_uDisplacement;
        uint8_t m_uLength;
        uint8_t m_uDecimalPlaces;
        uint8_t m_fFlags;

        uint32_t m_uNextValue;
        uint8_t m_uStepValue;
        char m_uReserved[8];
    };
#pragma pack(pop)

    nlohmann::json readDbfFile(std::string path);
}