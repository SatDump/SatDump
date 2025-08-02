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

            if(check_parity)
                logger->info("checking parity");
            else
                logger->info("not checking parity");

        }

        TelemetryReader::~TelemetryReader()
        {
        }

        // generated from CLASS data
        double avhrr_telemetry_coefficients_n15[22][6] = {
                {89.431184, 5.208287, 0.000000, 0.000000, 0.000000, 0.000000},             // Patch Temperature (K)
                {91.970208, 31.498950, 2.697024, 0.000000, 0.000000, 0.000000},            // Patch Temperature Extended (K)
                {0.000000, 0.000000, 2.000000, 0.000000, 0.000000, 0.000000},              // Patch Power (mW)
                {140.467696, 34.716048, 0.000000, 0.000000, 0.000000, 0.000000},           // Radiator Temperature (K)
                {3.441570, 8.167212, 0.034914, 0.000000, 0.000000, 0.000000},              // Blackbody Temperature 1 (C)
                {3.465314, 8.145576, 0.037730, 0.000000, 0.000000, 0.000000},              // Blackbody Temperature 2 (C)
                {3.514131, 8.155320, 0.037861, 0.000000, 0.000000, 0.000000},              // Blackbody Temperature 3 (C)
                {3.432581, 8.154563, 0.037861, 0.000000, 0.000000, 0.000000},              // Blackbody Temperature 4 (C)
                {-4.265896, 205.534096, -1.816705, 0.000000, 0.000000, 0.000000},          // Electronics Current (mA)
                {-2.694274, 59.815948, 0.000000, 0.000000, 0.000000, 0.000000},            // Motor Current (mA)
                {0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000},              // Earth Shield Position (?)
                {40.013320, -6.038373, 0.073978, 0.000000, 0.000000, 0.000000},            // Electronics Temperature (C)
                {33.781268, -7.130627, 0.062945, 0.000000, 0.000000, 0.000000},            // Cooler Housing Temperature (C)
                {35.005740, -8.766694, 0.563217, -0.072093, 0.000000, 0.000000},           // Baseplate Temperature (C)
                {34.184100, -7.482575, 0.000000, 0.000000, 0.000000, 0.000000},            // Motor Housing Temperature (C)
                {86.460240, -9.833891, 0.545627, -0.050068, 0.000000, 0.000000},           // A/D Converter Temperature (C)
                {-21.333300, 4.333330, 0.000000, 0.000000, 0.000000, 0.000000},            // Detector #4 Bias Voltage (V?)
                {-21.333300, 4.333330, 0.000000, 0.000000, 0.000000, 0.000000},            // Detector #5 Bias Voltage (V?)
                {-1326.222976, 1500.404992, -214.748365, 21.474836, -2.147484, 0.000000},  // Blackbody Temperature, Channel 3B (C)
                {72.703696, -15.004030, -1.169202, 0.000000, 0.000000, 0.000000},          // Blackbody Temperature, Channel 4 (C)
                {2147.483647, -2147.483648, 214.748365, -21.474836, 2.147484, -0.214748},  // Blackbody Temperature, Channel 5 (C)
                {0.000000, 1.400000, 0.000000, 0.000000, 0.000000, 0.000000},              // Reference Voltage (V?)
        };

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

        double n15_calibration(int value)
        {
            if(value < 0)
            {
                return -1;
            }

            // put value in range 0-1
            double x = (double)value/256;

            return 2.997558e-01 * pow(x,2.0) + 2.986854e+02 * x - 3.791725e-08;
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
            // double (*_coefficients)[6];

            // std::array<double, 6> _coefficients = coefficients["avhrr"]["Motor Current"].get<std::array<double, 6>>();

            // if (satnum == 1) // NOAA-15
            //     coefficients = avhrr_telemetry_coefficients_n15;
            // if (satnum == 2) // NOAA-18
            //     coefficients = avhrr_telemetry_coefficients_n18;
            // if (satnum == 3) // NOAA-19
            //     coefficients = avhrr_telemetry_coefficients_n19;

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

                // // AVHRR Electric Current (mA)
                // avhrr[8].push_back(telemetry_calibrate(_coefficients, analog2[frame][54]));
                // avhrr[8].push_back(telemetry_calibrate(_coefficients, analog2[frame][134]));
                // avhrr_timestamps[8].push_back(timestamp_160[frame]);
                // avhrr_timestamps[8].push_back(timestamp_160[frame] + 8);

                // // AVHRR Scan Motor Current (mA)
                // avhrr[9].push_back(telemetry_calibrate(_coefficients, analog2[frame][55]));
                // avhrr[9].push_back(telemetry_calibrate(_coefficients, analog2[frame][135]));
                // avhrr_timestamps[9].push_back(timestamp_160[frame]);
                // avhrr_timestamps[9].push_back(timestamp_160[frame] + 8);
            }

            // 32 second telemetry
            for(int frame=0; frame < timestamp_320.size(); frame++)
            {   
                for(int i=0; i < 19; i++)
                {
                    avhrr[value_locations_32[i][0]].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[value_locations_32[i][0]]].get<std::array<double, 6>>(), analog1[frame][value_locations_32[i][1]]));
                    avhrr_timestamps[value_locations_32[i][0]].push_back(timestamp_320[frame]);
                }

                // // Patch Power (mW)
                // avhrr[2].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[2]].get<std::array<double, 6>>(), analog1[frame][5]));
                // avhrr_timestamps[2].push_back(timestamp_320[frame]);

                // // Radiator Temperature (K)
                // avhrr[3].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[3]].get<std::array<double, 6>>(), analog1[frame][53]));
                // avhrr_timestamps[3].push_back(timestamp_320[frame]);

                // // Blackbody 1 Temperature (C)
                // avhrr[4].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[4]].get<std::array<double, 6>>(), analog1[frame][93]));
                // avhrr_timestamps[4].push_back(timestamp_320[frame]);

                // // Blackbody 2 Temperature (C)
                // avhrr[5].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[5]].get<std::array<double, 6>>(), analog1[frame][115]));
                // avhrr_timestamps[5].push_back(timestamp_320[frame]);

                // // Blackbody 3 Temperature (C)
                // avhrr[6].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[6]].get<std::array<double, 6>>(), analog1[frame][173]));
                // avhrr_timestamps[6].push_back(timestamp_320[frame]);
                
                // // Blackbody 4 Temperature (C)
                // avhrr[7].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[7]].get<std::array<double, 6>>(), analog1[frame][181]));
                // avhrr_timestamps[7].push_back(timestamp_320[frame]);

                // // Elec temp (C)
                // avhrr[11].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[11]].get<std::array<double, 6>>(), analog1[frame][58]));
                // avhrr_timestamps[11].push_back(timestamp_320[frame]);

                // // Cooler Housing Temp (C)
                // avhrr[12].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[12]].get<std::array<double, 6>>(), analog1[frame][61]));
                // avhrr_timestamps[12].push_back(timestamp_320[frame]);

                // // Cooler Housing Temp (C)
                // avhrr[12].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[12]].get<std::array<double, 6>>(), analog1[frame][61]));
                // avhrr_timestamps[12].push_back(timestamp_320[frame]);

                // avhrr[14].push_back(telemetry_calibrate(coefficients["avhrr"][avhrr_telemetry_names[14]].get<std::array<double, 6>>(), analog1[frame][86]));
                // avhrr_timestamps[14].push_back(timestamp_320[frame]);
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

            // for(int frame=0; frame < avhrr_timestamps[9].size(); frame++)
            // {

            //     telemetry["AVHRR Scan Motor Current"].push_back({avhrr_timestamps[9][frame],avhrr[9][frame]});
            //     telemetry["AVHRR Scan Motor Current"].push_back({avhrr_timestamps[9][frame] + 8,avhrr[9][frame]});
            // }

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
