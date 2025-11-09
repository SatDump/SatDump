#pragma once

#include <cstdint>
#include "nlohmann/json.hpp"

enum DownlinkMode : int
{
    LRPT,
    HRPT
};

// This is most likely temporary to some extent until
// more of this is figured out for calibration.
// Translations from Russians are definitely very imperfect,
// and this will need to be more data-oriented later!
inline void parseMSUMRTelemetry(nlohmann::json &msu_mr_telemetry, nlohmann::json &msu_mr_telemetry_calib, int linecnt, uint8_t *msumr_frame, DownlinkMode mode)
{
    if ((msumr_frame[12] & 0xF) == 0b0000)
        msu_mr_telemetry[linecnt]["msu_mr_set"] = "primary";
    else if ((msumr_frame[12] & 0xF) == 0b1111)
        msu_mr_telemetry[linecnt]["msu_mr_set"] = "backup";
    else
        msu_mr_telemetry[linecnt]["msu_mr_set"] = "Unknown";

    int mid = msumr_frame[12] >> 4;
    msu_mr_telemetry[linecnt]["msu_mr_id"] = mid;

    if ((mode == LRPT && (msumr_frame[13] & 0x0F) == 0x0F) || (mode == HRPT && msumr_frame[13] == 0b00001111)) // Analog TLM
    {
        const char *names[16 + 5] = {
            "Detector Temperature Channel 5", //"AF temperature of the 5th channel",
            "Detector Temperature Channel 6", //"AF temperature of the 6th channel",
            "Hot Body Temperature 1 (313K)",  //"Temperature AChT-G1",
            "Hot Body Temperature 2 (313K)",  //"Temperature AChT-G2",
            "Hot Body Temperature 3 (313K)",  //"Temperature AChT-G3",
            "Cold Body Temperature 1 (258K)", //"Temperature AChT-X1",
            "Cold Body Temperature 2 (258K)", //"Temperature AChT-X2",
            "Cold Body Temperature 3 (258K)", //"Temperature AChT-X3",
            "Lamp Current Channel 1",         //"Lamp current of calibration unit VK1",
            "Lamp Current Channel 2",         //"Lamp current of calibration unit VK2",
            "Lamp Current Channel 3",         //"Lamp current of calibration unit VK3",
            "IR Lenses temperatures",         //"Temperature of IR lenses of channels 4, 5, 6",
            "High voltage control on FP VK1",
            "High voltage control on FP VK2",
            "Detector Temperature Channel 3", //"FP temperature VK3",
            "Baseplate Temperature",          //    "Temperature of control point No. 1",
            "UKN1",
            "UKN2",
            "UKN3",
            "UKN4",
            "UKN5",
        };

        for (int i = 0; i < 8 /*+ 5*/; i++)
            msu_mr_telemetry[linecnt]["analog_tlm"][names[15 - i]] = ((uint8_t *)msumr_frame)[14 + i];

        for (int i = 8; i < 14 /*+ 5*/; i++)
            msu_mr_telemetry[linecnt]["analog_tlm"][names[15 - i]] = -int(((int8_t *)msumr_frame)[14 + i]) * 0.5 + 273.15; // Reverse-engineered! Seems good.

        msu_mr_telemetry_calib[linecnt]["analog_tlm"]["cold_temp1"] = msu_mr_telemetry[linecnt]["analog_tlm"]["Cold Body Temperature 1 (258K)"].get<double>() + (mid == 2 ? 40 : 0); // M2-2 patch
        msu_mr_telemetry_calib[linecnt]["analog_tlm"]["cold_temp2"] = msu_mr_telemetry[linecnt]["analog_tlm"]["Cold Body Temperature 2 (258K)"].get<double>() + (mid == 2 ? 40 : 0);
        msu_mr_telemetry_calib[linecnt]["analog_tlm"]["cold_temp3"] = msu_mr_telemetry[linecnt]["analog_tlm"]["Cold Body Temperature 3 (258K)"].get<double>() + (mid == 2 ? 40 : 0);
        msu_mr_telemetry_calib[linecnt]["analog_tlm"]["hot_temp1"] = msu_mr_telemetry[linecnt]["analog_tlm"]["Hot Body Temperature 1 (313K)"];
        msu_mr_telemetry_calib[linecnt]["analog_tlm"]["hot_temp2"] = msu_mr_telemetry[linecnt]["analog_tlm"]["Hot Body Temperature 2 (313K)"];
        msu_mr_telemetry_calib[linecnt]["analog_tlm"]["hot_temp3"] = msu_mr_telemetry[linecnt]["analog_tlm"]["Hot Body Temperature 3 (313K)"];

        for (int i = 14; i < 16 /*+ 5*/; i++)
            msu_mr_telemetry[linecnt]["analog_tlm"][names[15 - i]] = ((uint8_t *)msumr_frame)[14 + i];
    }
    else if ((mode == LRPT && (msumr_frame[13] & 0x0F) == 0x00) || (mode == HRPT && msumr_frame[13] == 0b00000000)) // Digital TLM
    {
        uint8_t *ptr = &msumr_frame[14];

        auto opModeToString = [](int mode) -> std::string
        {
            if (mode == 0b0001)
                return "Gain mode corresponding to model brightness B0";
            else if (mode == 0b0011)
                return "Gain mode corresponding to model brightness 0.5V0";
            else if (mode == 0b0100)
                return "Gain mode corresponding to model brightness 0.25V0";
            else if (mode == 0b0101)
                return "Discrete gain mode for brightness range";
            else if (mode == 0b0110)
                return "Linear transfer characteristic mode";
            else if (mode == 0b0111)
                return "TEST mode 1 (a gradation wedge is formed only in 4, 5, 6 channels)";
            else if (mode == 0b1000)
                return "TEST mode 2 (a gradation wedge is formed in all channels)";
            else if (mode == 0b1001)
                return "TEST mode 3 (video information is transmitted with 12-bit encoding and a reduced number of image elements in the channel for ground testing)";
            else
                return "Unknown Mode";
        };

        auto pHopModeToString = [](int mode) -> std::string
        {
            if (mode == 0)
                return "Norm";
            else if (mode == 1)
                return "Limit from above";
            else if (mode == 2)
                return "Restrict from below";
            else if (mode == 3)
                return "Reserve";
            else
                return "Unknown Mode";
        };

        msu_mr_telemetry[linecnt]["digital_tlm"]["channel1_mode"] = opModeToString(ptr[0] & 0xF);
        msu_mr_telemetry[linecnt]["digital_tlm"]["channel2_mode"] = opModeToString(ptr[0] >> 4);
        msu_mr_telemetry[linecnt]["digital_tlm"]["channel3_mode"] = opModeToString(ptr[1] & 0xF);
        msu_mr_telemetry[linecnt]["digital_tlm"]["channel4_mode"] = opModeToString(ptr[1] >> 4);
        msu_mr_telemetry[linecnt]["digital_tlm"]["channel5_mode"] = opModeToString(ptr[2] & 0xF);
        msu_mr_telemetry[linecnt]["digital_tlm"]["channel6_mode"] = opModeToString(ptr[2] >> 4);

        msu_mr_telemetry[linecnt]["digital_tlm"]["fp1_ic_photo4"] = pHopModeToString((ptr[3] >> 6) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp1_ic_photo3"] = pHopModeToString((ptr[3] >> 4) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp1_ic_photo2"] = pHopModeToString((ptr[3] >> 2) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp1_ic_photo1"] = pHopModeToString((ptr[3] >> 0) & 0b11);

        msu_mr_telemetry[linecnt]["digital_tlm"]["fp2_ik_photo4"] = pHopModeToString((ptr[4] >> 6) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp2_ik_photo3"] = pHopModeToString((ptr[4] >> 4) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp2_ik_photo2"] = pHopModeToString((ptr[4] >> 2) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp2_ik_photo1"] = pHopModeToString((ptr[4] >> 0) & 0b11);

        msu_mr_telemetry[linecnt]["digital_tlm"]["bos_operating_mode"] = ((ptr[5] >> 7) & 1) ? "expand" : "basic";
        msu_mr_telemetry[linecnt]["digital_tlm"]["leveling_channel_123"] = ((ptr[5] >> 6) & 1) ? "enabled" : "disabled";
        msu_mr_telemetry[linecnt]["digital_tlm"]["presenter_dns_sensor"] = ((ptr[5] >> 5) & 1) ? "included" : "reserve";
        msu_mr_telemetry[linecnt]["digital_tlm"]["synchronization_2"] = ((ptr[5] >> 4) & 1) ? "enabled" : "disabled";
        msu_mr_telemetry[linecnt]["digital_tlm"]["synchronization_1"] = ((ptr[5] >> 3) & 1) ? "enabled" : "disabled";
        msu_mr_telemetry[linecnt]["digital_tlm"]["included_pu_rg"] = ((ptr[5] >> 2) & 1) ? "included" : "disabled";
        msu_mr_telemetry[linecnt]["digital_tlm"]["protective_cover_2"] = ((ptr[5] >> 1) & 1) ? "open" : "closed";
        msu_mr_telemetry[linecnt]["digital_tlm"]["protective_cover_1"] = ((ptr[5] >> 0) & 1) ? "open" : "closed";

        msu_mr_telemetry[linecnt]["digital_tlm"]["fp3_ic_photo4"] = pHopModeToString((ptr[6] >> 6) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp3_ic_photo3"] = pHopModeToString((ptr[6] >> 4) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp3_ic_photo2"] = pHopModeToString((ptr[6] >> 2) & 0b11);
        msu_mr_telemetry[linecnt]["digital_tlm"]["fp3_ic_photo1"] = pHopModeToString((ptr[6] >> 0) & 0b11);
    }
}