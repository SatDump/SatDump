#include "telemetry_reader.h"
#include "logger.h"

namespace noaa
{
    namespace telemetry
    {
        TelemetryReader::TelemetryReader(int year, bool check_parity) : ttp(year),  check_parity(check_parity)
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
            for (int i=0; i<16; i++) {satcu_buf[i]=-1;}
            good_frames = 0;
        }

        TelemetryReader::~TelemetryReader()
        {
        }

        double telemetry_calibrate(std::array<double, 6> coefficients, int value)
        {
                // don't calibrate missing values
                if(value < 0)
                {
                        return std::nan("1");
                }

                // convert to voltage
                double voltage = (double)value * 0.02;

                // Use coefficients to get measurements
                // ax^5+bx^4+cx^3+dx^2+ex+f
                return coefficients[5]*pow(voltage,5) + coefficients[4]*pow(voltage,4) + coefficients[3]*pow(voltage,3) + coefficients[2]*pow(voltage,2) + coefficients[1] * voltage + coefficients[0];
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

            // TIP parity
            bool valid_frame = false;

            // copied from TIP-telem, which might be from PDT-TelemetryExplorer?
            if(check_parity)
            {
                bool parity[6] = {0,0,0,0,0,0};
                bool vparity[6];

                // compute parity for bits 1 - 5
                for(int i=0; i < 5; i++)
                {
                        for(int word=(i*17)+2; word < ((i+1)*17)+2; word++)
                        {
                                uint8_t byte = buffer[word];
                                for(char bit=0; bit<8; bit++)
                                {
                                        parity[i] ^= ((byte>>bit) & 0x01);
                                }
                        }

                }

                // compute parity for bit 6
                for(int word=87; word < 103; word++)
                {
                        uint8_t byte = buffer[word];
                        for(char bit=0; bit<8; bit++)
                        {
                                parity[5] ^= ((byte>>bit) & 0x01);
                        }
                }

                // get parity bits from frame
                uint8_t byte = buffer[103];
                for(char bit=1; bit<8; bit++)
                {
                        parity[5] ^= ((byte>>bit) & 0x01);
                }

                // check calculated parity
                for(int i=0; i < 6; i++)
                {
                        vparity[i] = (parity[i] == ((buffer[103] >> (5-i)) & 0x01));
                }

                // valid frame if *all* parity bits are valid, random noise will become a problem if only the required block is checked
                if(vparity[0] & vparity[1] & vparity[2] & vparity[3] & vparity[4] & vparity[5])
                {
                        valid_frame = true;
                }
            }
            else
            {
                // valid frame is parity checking is disabled
                valid_frame = true;
            }

            if (lastTS != -1)
                timestamp = lastTS + ((double)mf / 10.0);
            else
                timestamp = -1;

            frames++;

            // push telem rows to Vectors
            if(!(mf%320)) {
                last_timestamp = timestamp - 32;
                timestamp_320.push_back(last_timestamp);
                analog1.push_back(analog1_buf);
                for(int i=0; i<320; i++)
                {
                    analog1_buf[i]=-1;
                }
                major_frames++;
            }
            if(!(mf%160)) {
                last_timestamp = timestamp - 16;
                timestamp_160.push_back(last_timestamp);

                // Analog Subcom (16 sec)
                analog2.push_back(analog2_buf);
                for(int i=0; i<160; i++)
                {
                    analog2_buf[i]=-1;
                }

                // Analog Subcom-2 (16 sec)
                analog4.push_back(analog4_buf);
                for(int i=0; i<128; i++)
                {
                    analog4_buf[i]=-1;
                }

                // Solar Array Telemetry (SATCU)
                timestamp_satcu.push_back(last_timestamp);
                for(int i=0; i<16; i++)
                {
                    satcu_buf[i] = analog4_buf[128+i];
                    analog4_buf[128+i]=-1;
                }
                satcu.push_back(satcu_buf);
                timestamp_satcu.push_back(last_timestamp + 8);
                for(int i=0; i<16; i++)
                {
                    satcu_buf[i] = analog4_buf[144+i];
                    analog4_buf[144+i]=-1;
                }
                satcu.push_back(satcu_buf);
            }
            if(!(mf%32)) {
                last_timestamp = timestamp - 3.2;
                timestamp_32.push_back(last_timestamp);

                // Digital "B" Subcom-1
                digital1.push_back(digital1_buf);
                for(int i=0; i<256; i++)
                {
                    digital1_buf[i]=-1;
                }

                // Digital "B" Subcom-2
                digital2.push_back(digital2_buf);
                for(int i=0; i<256; i++)
                {
                    digital2_buf[i]=-1;
                }
            }
            if(!(mf%10)) {
                last_timestamp = timestamp - 1;
                timestamp_10.push_back(last_timestamp);

                // Analog Subcom (1 sec)
                analog3.push_back(analog3_buf);
                for(int i=0; i<10; i++)
                {
                    analog3_buf[i]=-1;
                }

                // DAU-1
                dau1.push_back(dau1_buf);
                for(int i=0; i<10; i++)
                {
                    dau1_buf[i]=-1;
                }

                // DAU-2
                dau2.push_back(dau2_buf);
                for(int i=0; i<10; i++)
                {
                    dau2_buf[i]=-1;
                }
            }

            if(valid_frame)
            {
                good_frames++;
                // get values from TIP frame
                uint8_t digital1 = buffer[8];
                uint8_t analog1 = buffer[9];
                uint8_t analog2 = buffer[10];
                uint8_t analog3 = buffer[11];
                uint8_t digital2 = buffer[12];
                uint8_t analog4 = buffer[13];
                uint8_t dau1 = buffer[14];
                uint8_t dau2 = buffer[15];

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
        }

        uint8_t value_locations_16[2][3] = {
            {8, 54, 134}, // Electronics Current (mA)
            {9, 55, 135} // Motor Current (mA)
        };

        // value, word pairs {(avhrr vector number), (subcom word)}
        // Blackbody Temperature for channels is not the same across satellite, only valid for N18
        uint8_t value_locations_32[19][2] = {
            {0, 45}, // Patch Temperature (K)
            {1, 29}, // Patch Temperature Extended (K)
            {2, 5}, // Patch Power (mW)
            {3, 53}, // Radiator Temperature (K)
            {4, 93}, // Blackbody Temperature 1 (C)
            {5, 165}, // Blackbody Temperature 2 (C)
            {6, 173}, // Blackbody Temperature 3 (C)
            {7, 181}, // Blackbody Temperature 4 (C)
            {11, 69}, // Electronics Temperature (C)
            {12, 61}, // Cooler Housing Temperature (C)
            {13, 77}, // Baseplate Temperature (C)
            {14, 85}, // Motor Housing Temperature (C)
            {15, 189}, // A/D Converter Temperature (C)
            {16, 21}, // Detector #4 Bias Voltage (V)
            {17, 247}, // Detector #5 Bias Voltage (V)
            {18, 197}, // Blackbody Temperature, Channel 3B (C)
            {19, 205}, // Blackbody Temperature, Channel 4 (C)
            {20, 252}, // Blackbody Temperature, Channel 5 (C)
            {21, 13} // Reference Voltage (V)
        };

        void TelemetryReader::calibration(nlohmann::json coefficients)
        {
            // 16/2 second telemetry
            for(int frame=0; frame < timestamp_160.size(); frame++)
            {
                for(int i=0; i < 2; i++)
                {
                    avhrr[value_locations_16[i][0]].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[value_locations_16[i][0]]].get<std::array<double, 6>>(), analog2[frame][value_locations_16[i][1]]));
                    avhrr[value_locations_16[i][0]].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[value_locations_16[i][0]]].get<std::array<double, 6>>(), analog2[frame][value_locations_16[i][2]]));
                    avhrr_timestamps[value_locations_16[i][0]].push_back(timestamp_160[frame]);
                    avhrr_timestamps[value_locations_16[i][0]].push_back(timestamp_160[frame] + 8);
                }
            }

            // 32 second telemetry
            for(int frame=0; frame < timestamp_320.size(); frame++)
            {   
                for(int i=0; i < 19; i++)
                {
                    // Not nice, but Channel BB temps are not the same for all satellites, so don't save
                    if ( i == 15 || i == 16 || i == 17 )
                    {
                        continue;
                    }
                    avhrr[value_locations_32[i][0]].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[value_locations_32[i][0]]].get<std::array<double, 6>>(), analog1[frame][value_locations_32[i][1]]));
                    avhrr_timestamps[value_locations_32[i][0]].push_back(timestamp_320[frame]);
                }
            }
        }

        nlohmann::json TelemetryReader::dump_telemetry()
        {
            // push telem rows to JSON
            // AVHRR
            for(int channel=0; channel < 22; channel++)
            {
                for (int i = 0; i < avhrr[channel].size(); i++)
                {
                    telemetry["AVHRR"][avhrr_telemetry_names[channel]].push_back({avhrr_timestamps[channel][i], avhrr[channel][i]});
                }
            }

            // Analog Subcom (32 sec)
            for(int frame=0; frame < analog1.size(); frame++)
            {
                //printf("debug %i", frame);
                telemetry["Analog 32 Second Subcom"]["timestamp"].push_back(timestamp_320[frame]);
                for(int i=0; i<320; i++)
                {
                    telemetry["Analog 32 Second Subcom"]["word " + std::to_string(i)].push_back(analog1[frame][i]);
                }
            }

            // Analog Subcom (16 sec)
            for(int frame=0; frame < analog2.size(); frame++)
            {
                telemetry["Analog 16 Second Subcom"]["timestamp"].push_back(timestamp_160[frame]);
                for(int i=0; i<160; i++)
                {
                    telemetry["Analog 16 Second Subcom"]["word " + std::to_string(i)].push_back(analog2[frame][i]);
                }
            }

            // Analog Subcom-2 (16 sec)
            for(int frame=0; frame < analog4.size(); frame++)
            {
                telemetry["Analog 16 Second Subcom 2"]["timestamp"].push_back(timestamp_160[frame]);
                for(int i=0; i<128; i++)
                {
                    telemetry["Analog 16 Second Subcom 2"]["word " + std::to_string(i)].push_back(analog4[frame][i]);
                }
            }

            for(int frame=0; frame < timestamp_satcu.size(); frame++)
            {
                // Solar Array Telemetry (SATCU)
                telemetry["Solar array"]["timestamp"].push_back(timestamp_satcu[frame]);
                for(int i=0; i<16; i++)
                {
                    telemetry["Solar array"]["word " + std::to_string(i)].push_back(satcu[frame][i]);
                }
            }

            for(int frame=0; frame < timestamp_10.size(); frame++)
            {

                // Analog Subcom (1 sec)
                telemetry["Analog 1 Second Subcom"]["timestamp"].push_back(timestamp_10[frame]);
                for(int i=0; i<10; i++)
                {
                    telemetry["Analog 1 Second Subcom"]["word " + std::to_string(i)].push_back(analog3[frame][i]);
                }

                // DAU-1
                telemetry["DAU-1"]["timestamp"].push_back(timestamp_10[frame]);
                for(int i=0; i<10; i++)
                {
                    telemetry["DAU-1"]["word " + std::to_string(i)].push_back(dau1[frame][i]);
                }

                // DAU-2
                telemetry["DAU-2"]["timestamp"].push_back(timestamp_10[frame]);
                for(int i=0; i<10; i++)
                {
                    telemetry["DAU-2"]["word " + std::to_string(i)].push_back(dau2[frame][i]);
                }
            }

            for(int frame=0; frame < timestamp_32.size(); frame++) {
                // Digital "B" Subcom-1
                telemetry["Digital Subcom 1"]["timestamp"].push_back(timestamp_32[frame]);
                for(int i=0; i<256; i++)
                {
                    telemetry["Digital Subcom 1"]["word " + std::to_string(i)].push_back(digital1[frame][i]);
                }

                // Digital "B" Subcom-2
                telemetry["Digital Subcom 2"]["timestamp"].push_back(timestamp_32[frame]);
                for(int i=0; i<256; i++)
                {
                    telemetry["Digital Subcom 2"]["word " + std::to_string(i)].push_back(digital2[frame][i]);
                }
            }

            return telemetry;
        }
    }
}
