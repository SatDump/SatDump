#pragma once

// GGAK-VE / GGAK-E space environment telemetry types.
// 224-byte CADUs: 4B ASM + 9B proprietary header + 209B payload + 2B checksum.
// NOT standard CCSDS AOS/TM.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace elektro_arktika
{
    namespace ggak
    {
        constexpr size_t ASM_SIZE = 4;
        constexpr size_t CADU_SIZE = 224;
        constexpr size_t HEADER_SIZE = 9;
        constexpr size_t PAYLOAD_SIZE = 209;
        constexpr size_t CHECKSUM_SIZE = 2;
        constexpr size_t PAYLOAD_OFFSET = ASM_SIZE + HEADER_SIZE; // byte 13

        constexpr uint8_t FILL_BYTE = 0x33;
        constexpr uint8_t FILL_BYTE_N2 = 0xAA;
        constexpr uint8_t FILL_BYTE_N5 = 0x33;

        constexpr int SYMBOL_RATE = 5000;
        constexpr int FINE_TIME_STEPS = 256;
        constexpr double TIME_TICK_SECONDS = 254.84;
        constexpr double FINE_TIME_STEP = TIME_TICK_SECONDS / FINE_TIME_STEPS;

        // FM-VE magnetometer: ADC offset 2339, scale 2.359 nT/count (IGRF at GEO)
        constexpr int MAG_READING_LEN = 15;
        constexpr int MAG_READINGS_PER_FRAME = 13;
        constexpr int MAG_ADC_OFFSET = 2339;
        constexpr double MAG_SCALE = 2.359;
        constexpr double MAG_BASELINE_NT = 103.0;
        constexpr double VOLTAGE_DIVISOR = 125.0;
        constexpr int MAG_RAW_SANITY = 10000;

        // Particle detectors (0x40 frames): GALS-VE ch1-7 + SKIF-VE MIP ch8
        constexpr uint8_t PARTICLE_MARKER_A0 = 0xA0;
        constexpr uint8_t PARTICLE_MARKER_A1 = 0xA1;
        constexpr int PARTICLE_READING_LEN = 19;
        constexpr int PARTICLE_CHANNELS = 8;
        constexpr int PARTICLE_READINGS_PER_FRAME = 11;

        // SKL-E spectral (0x5C / 0x50 frames)
        constexpr uint8_t SPECTRAL_MARKER = 0xAC;
        constexpr uint8_t SPECTRAL_CAL_MARKER = 0xA8;
        constexpr int SPECTRAL_MIN_PACKET_BYTES = 10;

        // SKIF-VE ESA energy sweep: 12 levels, 1 s/step (0x20 = V, 0x30 = G)
        constexpr int SKIF_VE_ESA_STEPS = 12;
        constexpr std::array<double, 12> SKIF_VE_ESA_ENERGIES_KEV = {
            0.05, 0.25, 0.45, 0.80, 1.30, 2.50,
            4.50, 7.50, 10.00, 13.00, 16.00, 20.00,
        };

        inline uint16_t read_be16(const uint8_t *p)
        {
            return (static_cast<uint16_t>(p[0]) << 8) | p[1];
        }

        inline int16_t read_be16s(const uint8_t *p)
        {
            return static_cast<int16_t>(read_be16(p));
        }

        inline bool is_fill_frame(uint8_t frame_type)
        {
            return frame_type == 0x77 || frame_type == 0x88;
        }

        inline bool is_fill_data(const uint8_t *data, size_t len, uint8_t fill_byte)
        {
            for (size_t i = 0; i < len; i++)
                if (data[i] != fill_byte)
                    return false;
            return true;
        }

        inline bool validate_checksum(const uint8_t *cadu)
        {
            uint32_t sum = 0;
            for (size_t i = 0; i < CADU_SIZE - CHECKSUM_SIZE; i++)
                sum += cadu[i];
            return (sum & 0xFFFF) == read_be16(cadu + CADU_SIZE - CHECKSUM_SIZE);
        }

        inline void format_timestamp(double elapsed_s, char *buf, size_t buf_size)
        {
            if (elapsed_s < 0.0)
                elapsed_s = 0.0;
            int days = static_cast<int>(elapsed_s / 86400.0);
            double rem = elapsed_s - days * 86400.0;
            int hours = static_cast<int>(rem / 3600.0);
            int mins = static_cast<int>(std::fmod(rem, 3600.0) / 60.0);
            int secs = static_cast<int>(std::fmod(rem, 60.0));
            std::snprintf(buf, buf_size, "D%03d %02d:%02d:%02d", days, hours, mins, secs);
        }

        inline const char *frame_type_name(uint8_t ft)
        {
            switch (ft)
            {
            case 0x00: return "Housekeeping-A";
            case 0x10: return "SKIF-VE SER summary";
            case 0x20: return "SKIF-VE/V ESA";
            case 0x30: return "SKIF-VE/G ESA";
            case 0x40: return "GALS-VE + SKIF-VE MIP";
            case 0x50: return "SKL-E calibration";
            case 0x5C: return "SKL-E spectral";
            case 0x60: return "Housekeeping-B (N2)";
            case 0x70: return "FM-VE Magnetometer";
            case 0x77: return "Fill";
            case 0x88: return "Fill-alt";
            default:   return "Unknown";
            }
        }

        inline double frame_cadence_seconds(uint8_t ft)
        {
            switch (ft)
            {
            case 0x5C: return 2 * FINE_TIME_STEP;
            case 0x20:
            case 0x30: return 3 * FINE_TIME_STEP;
            case 0x40: return 11 * FINE_TIME_STEP;
            case 0x70: return 13 * FINE_TIME_STEP;
            case 0x00:
            case 0x10: return 17 * FINE_TIME_STEP;
            default:   return 0.0;
            }
        }

        // Source ID: 0x30 = BND-E (GGAK-E, Elektro-L N1/N2), 0x31 = BND-VE (Elektro-L N3+, Arktika-M N1+)
        struct SatelliteProfile
        {
            const char *name;
            const char *generation;
            uint8_t source_id;
            uint8_t data_fill_byte;
            uint8_t skl_primary_type;
            uint8_t skl_marker;
        };

        constexpr SatelliteProfile PROFILE_BND_E = {
            "GGAK-E (Elektro-L N1/N2)", "BND-E", 0x30, FILL_BYTE_N2, 0x50, SPECTRAL_CAL_MARKER};

        constexpr SatelliteProfile PROFILE_BND_VE = {
            "GGAK-VE (Elektro-L N3+)", "BND-VE", 0x31, FILL_BYTE_N5, 0x5C, SPECTRAL_MARKER};

        constexpr uint8_t DEFAULT_SOURCE_ID = 0x31;

        // 9-byte proprietary header (bytes 4-12): frame_type, master/channel counter,
        // source_id (0x30/0x31/0x33), coarse_time (uint16 BE), fine_time (0-255)
        struct GGAKFrameHeader
        {
            uint8_t frame_type;
            uint16_t master_counter;
            uint16_t channel_counter;
            uint8_t source_id;
            uint16_t coarse_time;
            uint8_t fine_time;

            double elapsed_seconds() const
            {
                return coarse_time * TIME_TICK_SECONDS + fine_time * FINE_TIME_STEP;
            }
        };

        inline GGAKFrameHeader parse_header(const uint8_t *cadu)
        {
            return {
                cadu[4],
                read_be16(cadu + 5),
                read_be16(cadu + 7),
                cadu[9],
                read_be16(cadu + 10),
                cadu[12],
            };
        }

        // FM-VE magnetometer reading: 15 bytes per sample, up to 13 per 0x70 frame
        struct MagReading
        {
            uint16_t mag_total_raw;
            uint16_t mag_x_raw;
            uint16_t mag_y_raw;
            uint16_t mag_z_raw;
            uint16_t voltage_raw;
            uint16_t met_counter; // onboard elapsed-time counter (~1 tick/min), NOT temperature
            uint8_t status_flag;
            uint8_t mode;
            uint8_t mode2;
            uint32_t coarse_time;
            uint8_t fine_time;
            bool checksum_ok = true;

            double mag_total_nt() const { return (static_cast<int>(mag_total_raw) - MAG_ADC_OFFSET) * MAG_SCALE; }
            double mag_x_nt() const { return (static_cast<int>(mag_x_raw) - MAG_ADC_OFFSET) * MAG_SCALE; }
            double mag_y_nt() const { return (static_cast<int>(mag_y_raw) - MAG_ADC_OFFSET) * MAG_SCALE; }
            double mag_z_nt() const { return (static_cast<int>(mag_z_raw) - MAG_ADC_OFFSET) * MAG_SCALE; }
            double voltage() const { return voltage_raw / VOLTAGE_DIVISOR; }

            double elapsed_seconds() const
            {
                return coarse_time * TIME_TICK_SECONDS + fine_time * FINE_TIME_STEP;
            }

            bool is_valid() const
            {
                if (mag_x_raw > MAG_RAW_SANITY || mag_y_raw > MAG_RAW_SANITY ||
                    mag_z_raw > MAG_RAW_SANITY || mag_total_raw > MAG_RAW_SANITY)
                    return false;
                return status_flag != 0xFF;
            }

            int activity_level() const
            {
                double dev = std::abs(mag_total_nt() - MAG_BASELINE_NT);
                if (dev < 5.0) return 1;
                if (dev < 20.0) return 2;
                if (dev < 50.0) return 3;
                if (dev < 100.0) return 4;
                return 5;
            }

            const char *activity_label() const
            {
                switch (activity_level())
                {
                case 1: return "Normal";
                case 2: return "Slight";
                case 3: return "Moderate";
                case 4: return "Strong";
                default: return "Extreme";
                }
            }
        };

        // GALS-VE + SKIF-VE MIP: 11 readings per 0x40 frame, 19 bytes each, 8 channels
        struct ParticleReading
        {
            uint8_t sub_type;
            uint8_t counter;
            std::array<uint16_t, PARTICLE_CHANNELS> channels;
            uint32_t coarse_time;
            uint8_t fine_time;
            bool checksum_ok = true;

            double elapsed_seconds() const
            {
                return coarse_time * TIME_TICK_SECONDS + fine_time * FINE_TIME_STEP;
            }
        };

        // Generic sub-packet for ESA, SKL-E spectral, and housekeeping (int32_t channels)
        struct SubPacket
        {
            uint8_t seq;
            std::vector<int32_t> channels;
            uint32_t coarse_time;
            uint8_t fine_time;
            bool checksum_ok = true;

            double elapsed_seconds() const
            {
                return coarse_time * TIME_TICK_SECONDS + fine_time * FINE_TIME_STEP;
            }
        };

        struct CounterGapInfo
        {
            int total_frames;
            int gaps;
            int missing_estimate;
        };

    } // namespace ggak
} // namespace elektro_arktika
