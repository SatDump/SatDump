#include "telemetry_reader.h"

namespace noaa
{
    namespace telemetry
    {
        TelemetryReader::TelemetryReader(int year) : ttp(year)
        {
            // Set all row buffers to null (-1)
            for (int i=0; i<320; i++) {analog1_buf[i]=-1;}
            for (int i=0; i<160; i++) {analog2_buf[i]=-1;}
            for (int i=0; i<10; i++) {analog3_buf[i]=-1;}
            for (int i=0; i<160; i++) {analog4_buf[i]=-1;}
            for (int i=0; i<256; i++) {digital1_buf[i]=-1;}
            for (int i=0; i<256; i++) {digital2_buf[i]=-1;}
            for (int i=0; i<10; i++) {dau1_buf[i]=-1;}
            for (int i=0; i<10; i++) {dau2_buf[i]=-1;}
        }

        TelemetryReader::~TelemetryReader()
        {

        }

        void TelemetryReader::work(uint8_t *buffer)
        {
            uint16_t mf = ((buffer[4] & 1) << 8) | (buffer[5]);
            double timestamp;
            double last_timestamp;

            if (mf > 319)
                return;

            if (mf == 0){
                int days = (buffer[8] << 1) | (buffer[9] >> 7);
                uint64_t millisec = ((buffer[9] & 0b00000111) << 24) | (buffer[10] << 16) | (buffer[11] << 8) | buffer[12];
                lastTS = ttp.getTimestamp(days, millisec);
            }

            if (lastTS != -1)
                timestamp = lastTS + ((double)mf / 10.0);
            else
                timestamp = -1;

            // get values from TIP frame
            uint8_t digital1 = buffer[8];
            uint8_t analog1 = buffer[9];
            uint8_t analog2 = buffer[10];
            uint8_t analog3 = buffer[11];
            uint8_t digital2 = buffer[12];
            uint8_t analog4 = buffer[13];
            uint8_t dau1 = buffer[14];
            uint8_t dau2 = buffer[15];

            // push telem rows to JSON
            if(!(mf%320)) {
                last_timestamp = timestamp - 32;
                telemetry["Analog 32 Second Subcom"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<320; i++)
                {
                    telemetry["Analog 32 Second Subcom"]["word " + std::to_string(i)].push_back(analog1_buf[i]);
                    analog1_buf[i]=-1;
                }
            }
            if(!(mf%160)) {
                last_timestamp = timestamp - 16;
                // AVHRR Scan Motor Current (maybe only for NOAA-15)
                telemetry["AVHRR Scan Motor Current"].push_back({last_timestamp,analog2_buf[55]});
                telemetry["AVHRR Scan Motor Current"].push_back({last_timestamp + 8,analog2_buf[135]});

                // Analog Subcom (16 sec)
                telemetry["Analog 16 Second Subcom"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<160; i++)
                {
                    telemetry["Analog 16 Second Subcom"]["word " + std::to_string(i)].push_back(analog2_buf[i]);
                    analog2_buf[i]=-1;
                }

                // Analog Subcom-2 (16 sec)
                telemetry["Analog 16 Second Subcom 2"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<128; i++)
                {
                    telemetry["Analog 16 Second Subcom 2"]["word " + std::to_string(i)].push_back(analog4_buf[i]);
                    analog4_buf[i]=-1;
                }

                // Solar Array Telemetry (SATCU)
                telemetry["Solar array"]["timestamp"].push_back(last_timestamp);
                telemetry["Solar array"]["timestamp"].push_back(last_timestamp + 8);
                for(int i=0; i<16; i++)
                {
                    telemetry["Solar array"]["word " + std::to_string(i)].push_back(analog4_buf[128+i]);
                    telemetry["Solar array"]["word " + std::to_string(i)].push_back(analog4_buf[144+i]);
                    analog4_buf[128+i]=-1;
                    analog4_buf[144+i]=-1;
                }
            }
            if(!(mf%32)) {
                last_timestamp = timestamp - 3.2;
                // Digital "B" Subcom-1
                telemetry["Digital Subcom 1"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<256; i++)
                {
                    telemetry["Digital Subcom 1"]["word " + std::to_string(i)].push_back(digital1_buf[i]);
                    digital1_buf[i]=-1;
                }

                // Digital "B" Subcom-2
                telemetry["Digital Subcom 2"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<256; i++)
                {
                    telemetry["Digital Subcom 2"]["word " + std::to_string(i)].push_back(digital2_buf[i]);
                    digital2_buf[i]=-1;
                }
            }
            if(!(mf%10)) {
                last_timestamp = timestamp - 1;
                // Analog Subcom (1 sec)
                telemetry["Analog 1 Second Subcom"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<10; i++)
                {
                    telemetry["Analog 1 Second Subcom"]["word " + std::to_string(i)].push_back(analog3_buf[i]);
                    analog3_buf[i]=-1;
                }

                // DAU-1
                telemetry["DAU-1"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<10; i++)
                {
                    telemetry["DAU-1"]["word " + std::to_string(i)].push_back(dau1_buf[i]);
                    dau1_buf[i]=-1;
                }

                // DAU-2
                telemetry["DAU-2"]["timestamp"].push_back(last_timestamp);
                for(int i=0; i<10; i++)
                {
                    telemetry["DAU-2"]["word " + std::to_string(i)].push_back(dau2_buf[i]);
                    dau2_buf[i]=-1;
                }
            }

            // fill row buffers
            analog1_buf[mf%320] = analog1;
            analog2_buf[mf%160] = analog2;
            analog3_buf[mf%10] = analog3;
            analog4_buf[mf%160] = analog4;
            for(int i=0; i<8; i++)
            {
                digital1_buf[(mf%32) * 8 + i] = (digital1 >> i) & 0x01;
                digital2_buf[(mf%32) * 8 + i] = (digital2 >> i) & 0x01;
            }
            dau1_buf[mf%10] = dau1;
            dau2_buf[mf%10] = dau2;
        }

        nlohmann::json TelemetryReader::dump_telemetry()
        {
            return telemetry;
        }
    }
}
