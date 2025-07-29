#include "specific_proc.h"
#include "logger.h"

namespace satdump
{
    namespace xrit
    {
        void tryApplyMSGCalib(std::shared_ptr<products::ImageProduct> &pro)
        {
            if (!pro->has_calibration())
                return;

            auto calib_cfg = pro->get_calibration_raw();
            if (calib_cfg["vars"]["buffer"].is_null())
                return;

            nlohmann::json &sev_config = calib_cfg["vars"]["buffer"];

            for (auto &h : pro->images)
            {
                int i = h.abs_index;
                if (i < 3 || i == 11)
                    pro->set_channel_unit(i, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                else
                    pro->set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
                pro->set_channel_wavenumber(i, freq_to_wavenumber(299792458.0 / (sev_config["wavelengths"][i].get<double>())));
            }
        }

        void tryApplyMSG_HRVProcessing(std::shared_ptr<products::ImageProduct> &pro, image::Image &img, std::string *nameptr)
        {
            if (!pro->contents.contains("hrv_buffer"))
                return;

            logger->info("Processing HRV... Could take a bit.");

            int LowerEastColumnActual = pro->contents["hrv_buffer"]["LowerEastColumnActual"];
            int UpperSouthLineActual = pro->contents["hrv_buffer"]["UpperSouthLineActual"];
            int UpperEastColumnActual = pro->contents["hrv_buffer"]["UpperEastColumnActual"];

            int hrv_x_size = 3712 * 3;
            int hrv_y_size = img.height();

            // TODOREWORK? HRV on RSS seems wonky here... UpperSouthLineActual is always 0
            if (hrv_y_size == 4176 && UpperSouthLineActual == 0 && UpperEastColumnActual == 0)
                UpperSouthLineActual = hrv_y_size;

            image::Image img2(img.depth(), hrv_x_size, hrv_y_size, 1);

            for (int line = 0; line < hrv_y_size; line++)
            {
                for (int x = 0; x < hrv_x_size; x++)
                {
                    if (line + 4 > UpperSouthLineActual)
                    {
                        int xpos = x + UpperEastColumnActual;
                        if (xpos >= 0 && xpos < hrv_x_size)
                        {
                            if (x < img.width())
                            {
                                int _line = (hrv_y_size - 1) - line;
                                int _xpos = (hrv_x_size - 1) - xpos;
                                int _x = (img.width() - 1) - x;
                                img2.set(_line * hrv_x_size + _xpos, img.get(_line * img.width() + _x));
                            }
                        }
                    }
                    else
                    {
                        int xpos = x + LowerEastColumnActual;
                        if (xpos >= 0 && xpos < hrv_x_size)
                        {
                            if (x < img.width())
                            {
                                int _line = (hrv_y_size - 1) - line;
                                int _xpos = (hrv_x_size - 1) - xpos;
                                int _x = (img.width() - 1) - x;
                                img2.set(_line * hrv_x_size + _xpos, img.get(_line * img.width() + _x));
                            }
                        }
                    }
                }
            }

            img = img2;
            pro->contents.erase("hrv_buffer");

            try
            {
                if (nameptr != nullptr)
                    *nameptr = "12";
                pro->get_channel_image(11).channel_name = "12"; // Restore proper name
            }
            catch (std::exception &e)
            {
                // Nothing to log, expected to fail sometimes
            }
        }
    } // namespace xrit
} // namespace satdump