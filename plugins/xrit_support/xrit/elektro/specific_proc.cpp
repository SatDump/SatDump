#include "specific_proc.h"

namespace satdump
{
    namespace xrit
    {
        void tryApplyELEKTROCalib(std::shared_ptr<products::ImageProduct> &pro)
        {
            if (!pro->has_calibration())
                return;

            auto calib_cfg = pro->get_calibration_raw();
            if (calib_cfg["vars"]["buffer"].is_null())
                return;

            nlohmann::json &msugs_config = calib_cfg["vars"]["buffer"];

            for (auto &h : pro->images)
            {
                int i = h.abs_index;
                if (i < 3)
                    pro->set_channel_unit(i, CALIBRATION_ID_REFLECTIVE_RADIANCE);
                else
                    pro->set_channel_unit(i, CALIBRATION_ID_EMISSIVE_RADIANCE);
                pro->set_channel_wavenumber(i, freq_to_wavenumber(299792458.0 / (msugs_config["wavelengths"][i].get<double>())));
            }
        }
    } // namespace xrit
} // namespace satdump