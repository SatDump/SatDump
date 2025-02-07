#include "dbf_file.h"

#include "core/exception.h"
#include <fstream>

namespace dbf_file
{
    inline std::string removeExtraSpaces(std::string v)
    {
        while (v.size() > 1 && v[v.size() - 1] == ' ')
            v.erase(v.end() - 1);
        return v;
    }

    nlohmann::json readDbfFile(std::string path)
    {
        std::ifstream data_in(path, std::ios::binary);
        if (!data_in.good())
            throw satdump_exception("DBF File invalid!");

        // Read header
        DbfHeader header;
        data_in.read((char *)&header, sizeof(header));

        // Read record names, first row
        int row_size = 0;
        int largest_field_size = 0;
        std::vector<DbfRecord> recordNames;
        for (unsigned i = 0; i < header.m_uNumRecords; ++i)
        {
            char end;
            data_in.read(&end, 1);
            if (end == 0x0D)
                break;

            // corrupted file? Abort to avoid infinite loop
            if (i == header.m_uNumRecords)
                return {}; // TODOREWORK?

            recordNames.push_back(DbfRecord());
            DbfRecord &record = recordNames.back();

            memcpy(&record, &end, 1);
            data_in.read((char *)(&record) + 1, sizeof(DbfRecord) - 1);

            row_size += record.m_uLength;
            largest_field_size = std::max<size_t>(largest_field_size, record.m_uLength);
        }

        // Read all rows
        nlohmann::json v;

        int rec_n = 0;
        std::vector<char> vecBuffer(largest_field_size, 0);
        while (!data_in.eof())
        {
            char deleted;
            data_in.read(&deleted, 1);
            if (deleted == 0x2A)
            {
                data_in.seekg(row_size, std::ios_base::cur);
                continue;
            }

            if (data_in.fail())
                break;

            if (deleted == 0x1A) // end-of-file marker
                break;

            for (size_t i = 0; i < recordNames.size(); ++i)
            {
                DbfRecord &record = recordNames[i];

                data_in.read(&vecBuffer[0], record.m_uLength);
                //                out.write(&vecBuffer[0], record.m_uLength);
                std::string name = std::string(record.m_archName);
                std::string val = removeExtraSpaces(std::string(&vecBuffer[0], &vecBuffer[record.m_uLength]));
                v[rec_n][name] = val;
            }

            rec_n++;
        }

        return v;
    }
}