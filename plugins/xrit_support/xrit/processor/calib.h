#pragma once

/**
 * @file calib.h
 * @brief Generic xRIT calibration parsing
 */

#include "logger.h"
#include "products/image_product.h"
#include "utils/string.h"
#include "xrit/xrit_file.h"

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief Most xRIT missions use the standard calibration header. This
         * function parses it to insert calibration information on the channel.
         *
         * TODOREWORK This will most probably get moved over to each, and maybe load up wavelengths from files??
         * Also quite... crappy?
         *
         * @param pro product to work on
         * @param image_data_function_record xRIT calibration header
         * @param channel instrument channel ID
         * @param instrument_id the actual instrument channel ID
         */
        inline void addCalibrationInfoFunc(satdump::products::ImageProduct &pro, ImageDataFunctionRecord *image_data_function_record, std::string channel, std::string instrument_id)
        {
            if (image_data_function_record)
            {
                if (instrument_id == "goesn_imager" && std::stoi(channel) > 0 && std::stoi(channel) <= 5)
                {
                    const float goesn_imager_wavelength_table[5] = {
                        630, 3900, 6480, 10700, 13300,
                    };

                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / goesn_imager_wavelength_table[std::stoi(channel) - 1]);
                }
                else if (instrument_id == "abi" && channel.find_first_not_of("0123456789") == std::string::npos && std::stoi(channel) > 0 && std::stoi(channel) <= 16)
                {
                    const float goes_abi_wavelength_table[16] = {
                        470, 640, 860, 1380, 1610, 2260, 3900, 6190, 6950, 7340, 8500, 9610, 10350, 11200, 12300, 13300,
                    };

                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / goes_abi_wavelength_table[std::stoi(channel) - 1]);
                }
                else if (instrument_id == "ahi" && std::stoi(channel) > 0 && std::stoi(channel) <= 16)
                {
                    const float hima_ahi_wavelength_table[16] = {
                        470, 510, 640, 860, 1600, 2300, 3900, 6200, 6900, 7300, 8600, 9600, 10400, 11200, 12400, 13300,
                    };

                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / hima_ahi_wavelength_table[std::stoi(channel) - 1]);
                }
                else if (instrument_id == "ami")
                {
                    double wavelength_nm = std::stod(channel.substr(2, channel.size() - 1)) * 100;
                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / wavelength_nm);
                }
                else
                    pro.set_channel_wavenumber(pro.images.size() - 1, -1);

                // if (instrument_id == "ahi")
                // printf("Channel %s\n%s\n", channel.c_str(), image_data_function_record->datas.c_str());

                auto lines = splitString(image_data_function_record->datas, '\n');
                if (lines.size() < 4)
                {
                    lines = splitString(image_data_function_record->datas, '\r');
                    if (lines.size() < 4)
                    {
                        logger->error("Error parsing calibration info into lines!");
                        return;
                    }
                }

                // for (int i = 0; i < lines.size(); i++)
                //     logger->critical("%d => %s", i, lines[i].c_str());

                if (lines[0].find("HALFTONE:=") != std::string::npos)
                {
                    // GOES xRIT modes
                    if (instrument_id == "goesn_imager" || instrument_id == "abi")
                    {
                        if (lines[1] == "_NAME:=toa_lambertian_equivalent_albedo_multiplied_by_cosine_solar_zenith_angle")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_ALBEDO);
                        }
                        else if (lines[1] == "_NAME:=toa_brightness_temperature")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_EMISSIVE_RADIANCE);
                        }
                    }

                    // GK-2A xRIT modes
                    if (instrument_id == "ami")
                    {
                        int bits_for_calib = 8;
                        if (lines[0].find("HALFTONE:=10") != std::string::npos)
                            bits_for_calib = 10;

                        if (lines[2] == "_UNIT:=ALBEDO(%)")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo / 100.0});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"][channel] = bits_for_calib;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_ALBEDO);
                        }
                        else if (lines[2] == "_UNIT:=KELVIN")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"][channel] = bits_for_calib;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_EMISSIVE_RADIANCE);
                        }
                    }

                    // Himawari xRIT modes
                    if (instrument_id == "ahi")
                    {
                        int bits_for_calib = 8;
                        bool lut_has_1023 = false, lut_has_4095 = false, lut_has_16383 = false;
                        if (lines[2].find("_UNIT:=PERCENT") != std::string::npos || lines[2].find("_UNIT:=ALBEDO(%)") != std::string::npos)
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                {
                                    lut.push_back({val, valo / 100.0});
                                    lut_has_1023 |= (val == 1023);
                                    lut_has_4095 |= (val == 4095);
                                    lut_has_16383 |= (val == 16383);
                                }
                            }

                            if (lut_has_1023)
                                bits_for_calib = 10;
                            if (lut_has_4095)
                                bits_for_calib = 12;
                            if (lut_has_16383)
                                bits_for_calib = 14;

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"][channel] = bits_for_calib;
                            calib_cfg["to_complete"] = true;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_ALBEDO);
                        }
                        else if (lines[2].find("_UNIT:=KELVIN") != std::string::npos)
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                {
                                    lut.push_back({val, valo});
                                    lut_has_1023 |= (val == 1023);
                                    lut_has_4095 |= (val == 4095);
                                    lut_has_16383 |= (val == 16383);
                                }
                            }

                            if (lut_has_1023)
                                bits_for_calib = 10;
                            if (lut_has_4095)
                                bits_for_calib = 12;
                            if (lut_has_16383)
                                bits_for_calib = 14;

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"][channel] = bits_for_calib;
                            calib_cfg["to_complete"] = true;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_EMISSIVE_RADIANCE);
                        }
                    }
                }
            }
        }
    } // namespace xrit
} // namespace satdump