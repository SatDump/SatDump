#pragma once

#include <cstdint>
#include "common/utils.h"
#include <arpa/inet.h> // for ntohs() etc.
#include <stdint.h>

enum gvar_product_id
{
    NO_DATA,
    AAA_IR_DATA,
    AAA_VISIBLE_DATA,
    GVAR_IMAGER_DOCUMENATION,
    GVAR_IMAGER_IR_DATA,
    GVAR_IMAGER_VISIBLE_DATA,
    GVAR_SOUNDER_DOCUMENTATION,
    GVAR_SOUNDER_SCAN_DATA,
    GVAR_COMPENSATION_DATA,
    GVAR_TELEMTRY_STATISTICS,
    GVAR_AUX_TEXT,
    GIMTACTS_TEXT,
    SPS_TEXT,
    AAA_SOUNDING_PRODUCTS,
    GVAR_ECAL_DATA,
    GVAR_SPACELOOK_DATA,
    GVAR_BB_DATA,
    GVAR_CALIBRATION_COEFFICIENTS,
    GVAR_VISIBLE_NLUTS,
    GVAR_STAR_SENSE_DATA,
    IMAGER_FACTORY_COEFFICIENTS
};

#ifdef _WIN32
#pragma pack(push, 1)
#endif
struct bcd_date_t
{
    uint8_t YEAR100:4;
    uint8_t YEAR1000:4;

    uint8_t YEAR1:4;
    uint8_t YEAR10:4;

    uint8_t DOY10:4;
    uint8_t DOY100:4;

    uint8_t HOURS10:4;
    uint8_t DOY1:4;

    uint8_t MINUTES10:4;
    uint8_t HOURS1:4;

    uint8_t SECONDS10:4;
    uint8_t MINUTES1:4;

    uint8_t MSECONDS100:4;
    uint8_t SECONDS1:4;

    uint8_t MSECONDS1:4;
    uint8_t MSECONDS10:4;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
class be_uint16_t {
public:
        be_uint16_t() : be_val_(0) {
        }
        // Transparently cast from uint16_t
        be_uint16_t(const uint16_t &val) : be_val_(htons(val)) {
        }
        // Transparently cast to uint16_t
        operator uint16_t() const {
                return ntohs(be_val_);
        }
private:
        uint16_t be_val_;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
class be_uint32_t {
public:
        be_uint32_t() : be_val_(0) {
        }
        // Transparently cast from uint32_t
        be_uint32_t(const uint32_t &val) : be_val_((val)) {
        }
        // Transparently cast to uint32_t
        operator uint32_t() const {
                return (be_val_);
        }
private:
        uint32_t be_val_;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif


#ifdef _WIN32
#pragma pack(push, 1)
#endif
class be_float {
public:
        be_float() : be_val_(0) {
        }
        // Transparently cast from uint32_t
        be_float(const float &val) : be_val_((val)) {
        }
        // Transparently cast to uint32_t
        operator float() const {    
            //Gvar's float has 7 bits of exponant and 24 bits of mantissa. C's float has 8 bits of exponant and 23 bits of mantissa. 
            //Gvar's exponant moved the DP by 4 places, C's exponant moved DP by one place.
            uint32_t mantissa=be_val_;
            int8_t exp;
            uint8_t sign;
            float f;
            mantissa=htonl(mantissa);         //Reverse bytes

            sign=mantissa & 0x80000000;
            if(sign)
            {
                mantissa=-mantissa;
            }
            exp=mantissa >>24;  //Get exponant
            mantissa=mantissa & 0x00ffffff;    //Get just the mantissa 
            exp=exp&0x7f;//Remove sign
            exp=exp-64;//Remove exponant shift

            f=((float)mantissa/0x01000000) ;;
            float e=pow(0x10,exp);
            f=f*e;
            if(sign)
            {
                f=-f;
            }

            return f;
        }
private:
        uint32_t be_val_;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
class date_t {
public:
        date_t() : be_val_({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}) {
        }
        // Transparently cast to tm
        operator tm() const {
                tm r={0,0,0,0,0,0,0,0,0,0,0};
                r.tm_hour=be_val_.HOURS10*10+be_val_.HOURS1;
                r.tm_min = be_val_.MINUTES10 *10 + be_val_.MINUTES1;
                r.tm_sec = be_val_.SECONDS10 *10 + be_val_.SECONDS1;
                r.tm_year = (be_val_.YEAR1000 *1000 + be_val_.YEAR100*100+be_val_.YEAR10*10+be_val_.YEAR1)-1900;
                r.tm_yday = (be_val_.DOY100 *100 + be_val_.DOY10*10 + be_val_.DOY1)-1;
                r.tm_zone = "GMT";
                //Now to get day/month
                char d[30];
                strftime(d,30,"%Y-%j %H:%M:%S %Z",&r);
                strptime(d,"%Y-%j %H:%M:%S %Z",&r);
                
                return r;
        }
private:
        bcd_date_t be_val_;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif


#ifdef _WIN32
#pragma pack(push, 1)
#endif
struct PrimaryBlockHeader
{
    uint8_t block_id;
    uint8_t word_size;
    be_uint16_t word_count;
    be_uint16_t product_id;
    uint8_t repeat_flag;
    uint8_t version_number;
    uint8_t data_valid;
    uint8_t ascii_binary;
    uint8_t sps_id;
    uint8_t range_word;
    be_uint16_t block_count;
    uint16_t spare1;
    date_t time_code_bcd;
    uint32_t spare2;
    be_uint16_t header_crc;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
struct Block0Header
{
    uint8_t SPCID;
    uint8_t SPSID;
    uint32_t ISCAN_FRAME_START:1;
    uint32_t ISCAN_FRAME_END:1;    
    uint32_t ISCAN_FRAME_BREAK:1;    
    uint32_t ISCAN_PIXELS_LOST:1;    
    uint32_t ISCAN_PRI_1:1;    
    uint32_t ISCAN_PRI_2:1;    
    uint32_t ISCAN_EAST_TO_WEST:1;   
    uint32_t ISCAN_SOUTH_TO_NORTH:1;    
    uint32_t ISCAN_IMC_ACTIVE:1;    
    uint32_t ISCAN_LOST_HEADER:1;    
    uint32_t ISCAN_LOST_TRAILER:1;    
    uint32_t ISCAN_LOST_TELEM:1;    
    uint32_t ISCAN_TIME_BREAK:1;    
    uint32_t ISCAN_SIDE_1_ACTIVE:1;    
    uint32_t ISCAN_SIDE_2_ACTIVE:1;    
    uint32_t ISCAN_VIS_NORM_ACTIVE:1;    
    uint32_t ISCAN_IR_CALIB_ACTIVE:1;    
    uint32_t ISCAN_YAW_FLIPPED:1;    
    uint32_t ISCAN_IR_DATA_INVALID:6;    
    uint32_t ISCAN_VIS_DATA_INVALID:7;    
    uint8_t IDSUB[16];
    date_t TCURR; //Current SPS time
    date_t TCHED; //Time of current header block
    date_t TCTRL; //Time of current trailer block  
    date_t TLHED; //Time of lagged header block
    date_t TLTRL; //Time of lagged trailer block
    date_t TIPFS; //Time of priority frame start
    date_t TINFS; //Time of normal frame start
    date_t TISPC; //Time of last spacelook calibration
    date_t TIECL; //Time of last ECAL
    date_t TIBBC; //Time of last BB-Cal
    date_t TISTR; //Time of last star sense
    date_t TIRAN; //Time of last ranging measurement
    date_t TIIRT; //Time tag of current IR calibration set
    date_t TIVIT; //Time tag of current visible NLUT set
    date_t TCLMT; //Time tag of current Limits sets
    date_t TIONA; //Time tag current O&A set implemented
    be_uint16_t RISCT; //Relative output scan sequence count since frame start. Ranges 1-1974.
    be_uint16_t AISCT; //Absolute number of the current output scan. Values of 1-1974 correspond to output scans (northernmost to southernmost).
    be_uint16_t INSLN; //The number of the northernmost visible detector scan line in the current scan. Inclusive values of 1 to 15,780 correspond to detector lines (northernmost to southernmost).
    be_uint16_t IWFPX; //The number of the westernmost visible pixel in the current frame. Inclusive values of 1 to 30,677 correspond to the pixels (westernmost to easternmost).
    be_uint16_t IEFPX; //The number of the easternmost visible pixel in the current frame. Inclusive values of 4 to 30,680 correspond to the pixels (westernmost to easternmost).
    be_uint16_t INFLN; //The number of the northernmost visible detector scan line in the current frame. Inclusive values of 1-15,780 correspond to detector lines (northernmost to southernmost).
    be_uint16_t ISFLN; //The number of the southernmost visible detector scan line in the current frame. Inclusive values of 8 to 15,787 correspond to detector lines (northernmost to southernmost).
    be_uint16_t IMDPX; //The number of the visible pixel corresponding to an instrument azimuth of 0 E . Nominal value, ½ full range, is 15,340. This value is an instrument-specific constant.
    be_uint16_t IMDLN; //The number of the scan line corresponding to an instrument elevation of 0°. Nominal value, ½ full range, is 7894. This value is an instrument-specific constant.
    be_uint16_t IMDCT; //The number of the output scan corresponding to an instrument elevation of 0°. Nominal value, ½ full range, is 987. This value is an instrument-specific constant.
    be_uint16_t IGVLN; //The number of the visible detector scan line intersecting the subsatellite point.
    be_uint16_t IGVPX; //The number of the visible pixel intersecting the subsatellite point.
    be_float SUBLA; //The subsatellite point latitude value is a floating point number with units of degrees.
    be_float SUBLO; //The subsatellite point longitude value is a floating point number with units of degrees.
    uint8_t CZONE; //Current compensation zone (0–32) is an 8-bit integer number. A zero value indicates that no compensation is being performed. Values 1–32 denote the latitudinal zone for which corrections are applied.
    uint8_t V1PHY; //The physical detector number 1–8 assigned to GVAR Block 3.
    be_uint16_t G1CNT; //GRID 1 active entry count 0–512 is a 16-bit integer number. See words 2307–5386.
    be_uint16_t G2CNT; //GRID 2 active entry count 0–512 is a 16-bit integer number. See words 2307–5386.
    int16_t PBIAS; //E-W grid bias (0 ± 12546 pixels) is a signed 15-bit integer number denoting the pixel offset employed for the grid data. A value of zero indicates the grid is not shifted from the locations computed using the current O&A.
    int16_t LBIAS; //N-S grid bias (0 ± 7892 pixels) is similar to E-W grid bias, except in the N-S direction.
    uint8_t ISCP1; //Odd parity byte computed for words 3-4 (part of ISCAN).
    uint8_t Spare; //– not used
    float IDBER; //Current raw data Bit Error Rate (BER) is a floating point number denoting the most recent measure of raw data error rate. Nominal values are on the order of 1.0E–6.
    float RANGE; //Most recently computed range – a floating point value denoting the number of 50-MHz clock counts for signal transmission from satellite to ground.
    float GPATH; //Most recent range calibration ground path delay: a floating point value denoting the number of 50-MHz clock counts that the GVAR signal takes to transit through CDA station equipment.
    float XMSNE; //The call tower range calibration value – a floating point value denoting the number of 50-MHz clock counts that the GVAR signal takes to transit the satellite transmission electronics.
    bcd_date_t TGPAT; //CDA TOD of GPATH measurement, format as provided for words 23–30.
    bcd_date_t TXMSN; //CDA TOD of XMSNE measurement, format as provided for words 23–30.
    be_uint16_t ISTIM; //Current line scan time in integer msec computed as TCTRL – TCHED.
    uint8_t IFRAM; //Current frame counter. Integer ranging from 0 to 255 identifying current frame; rolls over to 0 following 255.
    uint8_t IMODE; //Current imaging mode. Integer value as follows:
    //1 = Routine
    //2 = Rapid scan operation
    //3 = Super rapid scan operation
    //4 = Checkout or special short-term operations
    //0, 5 -255 = Currently undefined.
    //The following four floating point values are in units of degrees. Off-Earth coordinates have a value of 999999.
    be_float IFNW1; //Current frame – northwest corner latitude
    be_float IFNW2; //Current frame – northwest corner longitude
    be_float IFSE1; //Current frame – southeast corner latitude
    be_float IFSE2; //Current frame – southeast corner longitude
    uint8_t IG2TN; //Second order gain interpolation table index number. Integer value of 1, 2, or 3 denoting which of the three possible tables is reported in IG2IT (see words 6779–7002).
    uint8_t ISCP2; //Repeat of ISCP1 (see word 193)
    uint16_t ISCA2; //Repeat of words 3-4 (part of ISCAN)
    date_t CIFST; //Current Imager Frame Start Time. CDA time tag - 8 words formatted in same manner as defined for words 23-150.
    uint8_t Spares[19]; //– not used
    uint8_t PXOR1; //Longitudinal parity XOR of words 1–277
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

struct LineDocumentationHeader
{
    uint16_t sc_id;
    uint16_t sps_id;
    uint16_t l_side;
    uint16_t detector_number;
    uint16_t source_channel;
    uint32_t relative_scan_count;
    uint16_t imager_scan_status_1;
    uint16_t imager_scan_status_2;
    uint16_t pixel_count;
    uint16_t word_count;
    uint16_t zonal_correction;
    uint16_t llag;
    uint16_t spare;

    LineDocumentationHeader(uint8_t *data)
    {
        uint16_t data_buffer[16];

        int pos = 0;
        for (int i = 0; i < 16; i += 4)
        {
            data_buffer[i] = (data[pos + 0] << 2) | (data[pos + 1] >> 6);
            data_buffer[i + 1] = ((data[pos + 1] % 64) << 4) | (data[pos + 2] >> 4);
            data_buffer[i + 2] = ((data[pos + 2] % 16) << 6) | (data[pos + 3] >> 2);
            data_buffer[i + 3] = ((data[pos + 3] % 4) << 8) | data[pos + 4];
            pos += 5;
        }

        sc_id = data_buffer[0];
        sps_id = data_buffer[1];
        l_side = data_buffer[2];
        detector_number = data_buffer[3];
        source_channel = data_buffer[4];
        relative_scan_count = data_buffer[5] << 10 | data_buffer[6];
        imager_scan_status_1 = data_buffer[7];
        imager_scan_status_2 = data_buffer[8];
        pixel_count = data_buffer[9] << 10 | data_buffer[10];
        word_count = data_buffer[11] << 10 | data_buffer[12];
        zonal_correction = data_buffer[13];
        llag = data_buffer[14];
        spare = data_buffer[15];
    }
};