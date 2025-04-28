#include "projection.h"
#include "logger.h"

#include "common/geodetic/vincentys_calculations.h" // TODOREWORK MOVE OUT
#include "common/projection/projs2/proj_json.h"
#include "raytrace/gcp_compute.h"

namespace satdump
{
    namespace proj
    {
        Projection::Projection() {}

        Projection::~Projection()
        {
            if (fwd_type == PROJ_STANDARD || inv_type == PROJ_STANDARD)
                ::proj::projection_free(&std_proj);
        }

        bool Projection::init(bool fwd, bool inv)
        {
            ///////////////////////////////////////////////////////////
            // We need image width/height
            ///////////////////////////////////////////////////////////
            if (d_cfg.contains("width"))
                width = d_cfg["width"];
            else
                throw satdump_exception("Image width must be present!");

            if (d_cfg.contains("height"))
                height = d_cfg["height"];
            else
                throw satdump_exception("Image height must be present!");

            ///////////////////////////////////////////////////////////
            // Get channel pre-transform, if present
            ///////////////////////////////////////////////////////////
            if (d_cfg.contains("transform"))
                transform = d_cfg["transform"];
            else
                transform.init_none();

            ///////////////////////////////////////////////////////////
            // Get optional second channel pre-transform, if present
            ///////////////////////////////////////////////////////////
            if (d_cfg.contains("transform2"))
            {
                has_2nd_transform = true;
                transform2 = d_cfg["transform2"];
            }

            ///////////////////////////////////////////////////////////
            // Attempt to setup a standard projection first
            ///////////////////////////////////////////////////////////
            try
            {
                std_proj = d_cfg;

                if (!::proj::projection_setup(&std_proj))
                {
                    fwd_type = inv_type = PROJ_STANDARD;
                    if (d_cfg.contains("proj_timestamp"))
                    {
                        logger->warn("Using projection timestamps for timestamp feedback. May be inacurate!");
                        proj_timestamp = d_cfg["proj_timestamp"];
                    }
                    return true;
                }
            }
            catch (std::exception &)
            {
                logger->trace("Not a standard projection!");
            }

            ///////////////////////////////////////////////////////////
            // If this didn't work, we can attempt a raytraced proj
            ///////////////////////////////////////////////////////////
            try
            {
                raytracer = get_satellite_raytracer(d_cfg);
                if (raytracer)
                {
                    fwd_type = PROJ_INVALID;
                    inv_type = PROJ_RAYTRACER;

                    if (fwd)
                    {
                        logger->critical("Forward on raytrace is imperfect!");
                        auto gcps = compute_gcps(d_cfg);
                        tps_fwd = std::make_shared<satdump::proj::LatLonTpsProjHelper>(gcps, 1, 0);
                        fwd_type = PROJ_THINPLATESPLINE;
                    }
                }
                return true;
            }
            catch (std::exception &e)
            {
                logger->trace("Not a raytraced projection! : %s", e.what());
            }

            ///////////////////////////////////////////////////////////
            // And finally, the special case of simple GCPs
            ///////////////////////////////////////////////////////////
            if (d_cfg["type"] == "normal_gcps")
                logger->critical("GCPs ALONE SUPPORT TBD!");

            return false;
        }

        bool Projection::forward(geodetic::geodetic_coords_t pos, double &x, double &y)
        {
            if (fwd_type == PROJ_STANDARD)
            {
                pos.toDegs(); // TODOREWORK?
                if (::proj::projection_perform_fwd(&std_proj, pos.lon, pos.lat, &x, &y))
                    return 1;
            }
            else if (fwd_type == PROJ_THINPLATESPLINE)
            {
                // Perform TPS
                tps_fwd->forward(pos, x, y);
                return 0; // We do NOT want to run the ChannelTransform in reverse, TPS takes care of it already!
            }
            else
            {
                throw satdump_exception("Invalid forward projection type! " + d_cfg["type"].get<std::string>());
            }

            // Run channel transform, might do nothing if no transform is needed
            transform.reverse(&x, &y);
            if (has_2nd_transform)
                transform2.reverse(&x, &y);
            return 0;
        }

        bool Projection::inverse(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            // Run channel transform, might do nothing if no transform is needed
            if (has_2nd_transform)
                transform2.forward(&x, &y);
            transform.forward(&x, &y);

            if (inv_type == PROJ_STANDARD)
            {
                pos.toDegs(); // TODOREWORK?
                if (otime != nullptr)
                    *otime = proj_timestamp;
                return ::proj::projection_perform_inv(&std_proj, x, y, &pos.lon, &pos.lat);
            }
            else if (inv_type == PROJ_RAYTRACER)
            {
                return raytracer->get_position(x, y, pos, otime);
            }
            else
            {
                throw satdump_exception("Invalid inverse projection type! " + d_cfg["type"].get<std::string>());
            }
        }

        void Projection::to_json(nlohmann::json &j) const
        {
            j = d_cfg;
            if (height != -1)
                j["height"] = height;
            if (width != -1)
                j["width"] = width;
        }

        void Projection::from_json(const nlohmann::json &j)
        {
            d_cfg = j; // TODOREWORK de-init?
            height = j.contains("height") ? j["height"].get<int>() : -1;
            width = j.contains("width") ? j["width"].get<int>() : -1;
            fwd_type = PROJ_INVALID;
            inv_type = PROJ_INVALID;
        }
    } // namespace proj
} // namespace satdump