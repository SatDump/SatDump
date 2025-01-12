#include "projection.h"
#include "logger.h"

#include "common/geodetic/vincentys_calculations.h" // TODOREWORK MOVE OUT
#include "raytrace/gcp_compute.h"

namespace satdump
{
    namespace warp // TODOREWORK REWRITE / MOVE OUT
    {
        double lon_shift(double lon, double shift);
        void shift_latlon_by_lat(double *lat, double *lon, double shift);

        void computeGCPCenter(std::vector<satdump::projection::GCP> &gcps, double &lon, double &lat);
        std::shared_ptr<projection::VizGeorefSpline2D> initTPSTransform(std::vector<projection::GCP> gcps, int shift_lon, int shift_lat);
    }

    namespace proj
    {

        namespace
        {
            std::shared_ptr<satdump::projection::VizGeorefSpline2D> setupTPSTransform(std::vector<projection::GCP> gcps, double &out_slat, double &out_slon)
            { // TODOREWORK REWRITE / MOVE OUT to avoid duplication!
                int shift_lon = 0, shift_lat = 0;

                // Calculate center, and handle longitude shifting
                {
                    double center_lat = 0, center_lon = 0;
                    warp::computeGCPCenter(gcps, center_lon, center_lat);
                    shift_lon = -center_lon;
                    shift_lat = 0;
                }

                // Check for GCPs near the poles. If any is close, it means this segment needs to be handled as a pole!
                for (auto &gcp : gcps)
                {
                    auto south_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(-90, 0, 0));
                    auto north_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(90, 0, 0));
#define MIN_POLE_DISTANCE 1000 // Maximum distance at which to consider we're working over a pole
                    if (south_dis.distance < MIN_POLE_DISTANCE)
                    {
                        shift_lon = 0;
                        shift_lat = -90;
                    }
                    if (north_dis.distance < MIN_POLE_DISTANCE)
                    {
                        shift_lon = 0;
                        shift_lat = 90;
                    }
                }

                //                shift_lat = shift_lon = 0;
                out_slat = shift_lat;
                out_slon = shift_lon;
                return warp::initTPSTransform(gcps, shift_lon, shift_lat);
            }
        }

        Projection::Projection()
        {
        }

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
            int width = -1, height = -1;

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
            // Attempt to setup a standard projection first
            ///////////////////////////////////////////////////////////
            try
            {
                std_proj = d_cfg;

                if (!::proj::projection_setup(&std_proj))
                {
                    fwd_type = inv_type = PROJ_STANDARD;
                    return true;
                }
            }
            catch (std::exception &)
            {
                logger->trace("Not a standard projection!");
            }

            ///////////////////////////////////////////////////////////
            // If this didn't work, we can attent a raytraced proj
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
                        tps_fwd = setupTPSTransform(gcps, tps_fwd_shift_lat, tps_fwd_shift_lon);
                        fwd_type = PROJ_THINPLATESPLINE;
                    }
                }
                return true;
            }
            catch (std::exception &)
            {
                logger->trace("Not a raytraced projection!");
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
                warp::shift_latlon_by_lat(&pos.lat, &pos.lon, tps_fwd_shift_lat);
                tps_fwd->get_point(warp::lon_shift(pos.lon, tps_fwd_shift_lon), pos.lat, tps_fwd_xy);
                x = tps_fwd_xy[0];
                y = tps_fwd_xy[1];
                return 0; // We do NOT want to run the ChannelTransform in reverse, TPS takes care of it already!
            }
            else
            {
                throw satdump_exception("Invalid forward projection type! " + d_cfg["type"].get<std::string>());
            }

            // Run channel transform, might do nothing if no transform is needed
            transform.reverse(&x, &y);
            return 0;
        }

        bool Projection::inverse(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            // Run channel transform, might do nothing if no transform is needed
            transform.forward(&x, &y);

            if (inv_type == PROJ_STANDARD)
            {
                pos.toDegs(); // TODOREWORK?
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
        }

        void Projection::from_json(const nlohmann::json &j)
        {
            d_cfg = j; // TODOREWORK de-init?
            fwd_type = PROJ_INVALID;
            inv_type = PROJ_INVALID;
        }
    }
}