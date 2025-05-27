#include "module_orbcomm_plotter.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/repack.h"
#include "common/geodetic/ecef_to_eci.h"
#include "common/geodetic/lla_xyz.h"
#include "core/config.h"
#include "utils/time.h"
#include "utils/color.h"

// We need access to libpredict internals!
#include "libs/predict/predict.h"
extern "C"
{
    void observer_calculate(const predict_observer_t *observer, double time, const double pos[3], const double vel[3], struct predict_observation *result);
}

// Probably to put in another .h
namespace
{
    // Define GPS leap seconds
    uint64_t leap_seconds[] = {46828800, 78364801, 109900802, 173059203, 252028804, 315187205, 346723206, 393984007, 425520008, 457056009, 504489610, 551750411, 599184012, 820108813, 914803214, 1025136015, 1119744016, 1167264017};
    int leapLen = 18;

    // Test to see if a GPS second is a leap second
    bool isleap(uint64_t gpsTime)
    {
        bool isLeap = false;
        for (int i = 0; i < leapLen; i++)
            if (gpsTime == leap_seconds[i])
                isLeap = true;
        return isLeap;
    }

    // Count number of leap seconds that have passed
    int countleaps(uint64_t gpsTime, bool to_gps)
    {
        int nleaps = 0; // number of leap seconds prior to gpsTime
        for (int i = 0; i < leapLen; i++)
        {
            if (!to_gps)
            {
                if (gpsTime >= leap_seconds[i] - i)
                    nleaps++;
            }
            else if (to_gps)
            {
                if (gpsTime >= leap_seconds[i])
                    nleaps++;
            }
        }
        return nleaps;
    }

    // Convert GPS Time to Unix Time
    time_t gps2unix(uint64_t gpsTime)
    {
        // Add offset in seconds
        time_t unixTime = gpsTime + 315964800;
        int nleaps = countleaps(gpsTime, false);
        unixTime = unixTime - nleaps;
        if (isleap(gpsTime))
            unixTime = unixTime + 0.5;
        return unixTime;
    }

    time_t gps_time_to_unix(uint64_t gps_weeks, uint64_t gps_week_time)
    {
        return gps2unix(gps_weeks * 604800 + gps_week_time);
    }

}

namespace orbcomm
{
    OrbcommPlotterModule::OrbcommPlotterModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    std::vector<ModuleDataType> OrbcommPlotterModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> OrbcommPlotterModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    int orbcomm_fcs(uint8_t *data, int len)
    {
        int i;
        unsigned char c0 = 0;
        unsigned char c1 = 0;
        for (i = 0; i < len; i++)
        {
            c0 = c0 + *(data + i);
            c1 = c1 + c0;
        }
        return ((long)(c0 + c1)); /* returns zero if buffer is error-free*/
    }

    double orbcomm_calcFreq(int f, bool small = true)
    {
        if (small)
        {
            if (f <= 0x40)
                f = 0x1 << 8 | f;
            else if (f >= 0x50)
                f = 0x0 << 8 | f;
        }
        return 137.0 + double(f) * 0.0025;
    }

    void OrbcommPlotterModule::process()
    {
        std::ifstream data_in;
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        std::ofstream data_out(d_output_file_hint + "_plotter.frm", std::ios::binary);
        logger->info("Using input frames " + d_input_file);
        uint8_t frm[600];

        double qth_lon = satdump::config::main_cfg["satdump_general"]["qth_lon"]["value"].get<double>();
        double qth_lat = satdump::config::main_cfg["satdump_general"]["qth_lat"]["value"].get<double>();
        double qth_alt = satdump::config::main_cfg["satdump_general"]["qth_alt"]["value"].get<double>();

        predict_observer_t *predict_obs = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt);

        time_t lastTime = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)frm, 600);
            else
                input_fifo->read((uint8_t *)frm, 600);

            data_out.write((char *)frm, 600);

            for (int i = 0; i < 50; i++)
            {
                if (frm[i * 12] == 0x1F)
                {
                    if (orbcomm_fcs(&frm[i * 12], 24) == 0)
                    {
                        std::reverse(&frm[i * 12 + 2], &frm[i * 12 + 22]);

                        int scid = frm[i * 12 + 1];
                        int week_number = frm[i * 12 + 2] << 8 | frm[i * 12 + 3];
                        int time_of_week = frm[i * 12 + 4] << 16 | frm[i * 12 + 5] << 8 | frm[i * 12 + 6];

                        // logger->info("Week Number %d, Time Of Week %d", week_number, time_of_week);

                        const double MAX_R_SAT = 8378155.0;
                        const double VAL_20_BITS = 1048576.0;
                        const double MAX_V_SAT = 7700.0;

                        uint32_t values[6];
                        repackBytesTo20bits(&frm[i * 12 + 7], 15, values);

                        long x_raw = values[5];
                        long y_raw = values[4];
                        long z_raw = values[3];

                        long x_raw2 = values[2];
                        long y_raw2 = values[1];
                        long z_raw2 = values[0];

                        double X = ((2.0 * x_raw * MAX_R_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                        double Y = ((2.0 * y_raw * MAX_R_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                        double Z = ((2.0 * z_raw * MAX_R_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;

                        double X_DOT = ((2.0 * x_raw2 * MAX_V_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                        double Y_DOT = ((2.0 * y_raw2 * MAX_V_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                        double Z_DOT = ((2.0 * z_raw2 * MAX_V_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;

                        geodetic::geodetic_coords_t lla;
                        xyz2lla({X, Y, Z}, lla);
                        lla.toDegs();

                        // Az/El
                        double az, el, range;
                        {
                            double pos[3];
                            pos[0] = X * 1000.0;
                            pos[1] = Y * 1000.0;
                            pos[2] = Z * 1000.0;

                            double vel[3];
                            vel[0] = X_DOT * 1000.0;
                            vel[1] = Y_DOT * 1000.0;
                            vel[2] = Z_DOT * 1000.0;

                            ecef_epehem_to_eci(0, pos[0], pos[1], pos[2], vel[0], vel[1], vel[2]);

                            predict_observation observ;
                            observer_calculate(predict_obs, predict_to_julian(0) + 2444238.5, pos, vel, &observ);

                            az = observ.azimuth * RAD_TO_DEG;
                            el = observ.elevation * RAD_TO_DEG;
                            range = observ.range;
                        }

                        time_t ephem_time = gps_time_to_unix(week_number, time_of_week);

                        logger->info("SCID %d, Week Number %d, Time Of Week %d, Time %s - X %.3d Y %.3d Z %.3d - X %.3f Y %.3f Z %.3f - Lon %.1f, Lat %.1f, Alt %.1f - Az %.1f El %.1f Range %.1f",
                                     scid + 70, week_number, time_of_week, satdump::timestamp_to_string(ephem_time).c_str(),
                                     x_raw, y_raw, z_raw,
                                     X_DOT, Y_DOT, Z_DOT,
                                     lla.lon, lla.lat, lla.alt,
                                     az, el, range);

#if 0
                        // Polar Plot!
                        // Draw the current satellite position
                        if (el > 0)
                        {
                            float point_x = plot_size / 2;
                            float point_y = plot_size / 2;

                            point_x += az_el_to_plot_x(plot_size, radius, az, el);
                            point_y -= az_el_to_plot_y(plot_size, radius, az, el);

                            uint8_t color[3];
                            hsv_to_rgb(fmod(scid, 10) / 10.0, 1, 1, color);
                            img.draw_circle(point_x, point_y, 2, color, true);
                        }
#endif

                        all_ephem_points_mtx.lock();
                        all_ephem_points.push_back({ephem_time, scid, (float)az, (float)el});

                        int contained = -1;
                        for (int c = 0; c < (int)last_ephems.size(); c++)
                            if (last_ephems[c].scid == scid)
                                contained = c;

                        if (contained != -1)
                            last_ephems[contained] = OrbComEphem{ephem_time, scid, (float)az, (float)el};
                        else
                            last_ephems.push_back(OrbComEphem{ephem_time, scid, (float)az, (float)el});

                        std::sort(last_ephems.begin(), last_ephems.end(), [](const auto &v1, const auto &v2)
                                  { return v1.time > v2.time; });

                        all_ephem_points_mtx.unlock();
                    }
                }
                else if (frm[i * 12] == 0x65)
                {
                    if (orbcomm_fcs(&frm[i * 12], 24) == 0)
                    {
                        int f = frm[i * 12 + 5];
                        logger->info("Synchronization, Freq %f Mhz", orbcomm_calcFreq(f));
                    }
                }
                else if (frm[i * 12] == 0x1C)
                {
                    if (orbcomm_fcs(&frm[i * 12], 12) == 0)
                    {
                        int pos = frm[i * 12 + 1] & 0xF;

                        std::reverse(&frm[i * 12 + 2], &frm[i * 12 + 10]);
                        shift_array_left(&frm[i * 12 + 2], 8, 4, &frm[i * 12 + 2]);
                        uint16_t values[5];
                        repackBytesTo12bits(&frm[i * 12 + 2], 8, values);

                        std::string frequencies = "";
                        for (int i = 0; i < 5; i++)
                            if (values[i] != 0)
                                frequencies += std::to_string(orbcomm_calcFreq(values[i], false)) + " Mhz, ";
                        logger->info("Channels %d : %s", pos, frequencies.c_str());
                    }
                }
            }

            progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        data_in.close();
    }

    inline float az_el_to_plot_x(float plot_size, float radius, float az, float el) { return sin(az * DEG_TO_RAD) * plot_size * radius * ((90.0 - el) / 90.0); }
    inline float az_el_to_plot_y(float plot_size, float radius, float az, float el) { return cos(az * DEG_TO_RAD) * plot_size * radius * ((90.0 - el) / 90.0); }

    void OrbcommPlotterModule::drawUI(bool window)
    {
        ImGui::Begin("Orbcomm Plotter", NULL, window ? 0 : NOWINDOW_FLAGS);

        time_t ctime = time(0);

        ImGui::Checkbox("Window##orbcomm", &should_be_a_window);

        if (should_be_a_window)
        {
            ImGui::End();
            ImGui::Begin("Orbcomm Plotter (Plot)");
        }

        ImGui::BeginGroup();
        {
            int d_pplot_size = (should_be_a_window ? 400 : 200) * ui_scale; // ImGui::GetWindowContentRegionMax().x;
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + d_pplot_size),
                                     style::theme.widget_bg);

            // Draw the "target-like" plot with elevation rings
            float radius = 0.45;
            float radius1 = d_pplot_size * radius * (3.0 / 9.0);
            float radius2 = d_pplot_size * radius * (6.0 / 9.0);
            float radius3 = d_pplot_size * radius * (9.0 / 9.0);

            draw_list->AddCircle({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                  ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                                 radius1, style::theme.green, 0, 2);
            draw_list->AddCircle({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                  ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                                 radius2, style::theme.green, 0, 2);
            draw_list->AddCircle({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                  ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                                 radius3, style::theme.green, 0, 2);

            draw_list->AddLine({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                ImGui::GetCursorScreenPos().y},
                               {ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                ImGui::GetCursorScreenPos().y + d_pplot_size},
                               style::theme.green, 2);
            draw_list->AddLine({ImGui::GetCursorScreenPos().x,
                                ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                               {ImGui::GetCursorScreenPos().x + d_pplot_size,
                                ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                               style::theme.green, 2);

            all_ephem_points_mtx.lock();
            for (auto &ephem : all_ephem_points)
            {
                // Draw the current satellite position
                if (ephem.el > 0)
                {
                    float point_x = ImGui::GetCursorScreenPos().x + (d_pplot_size / 2);
                    float point_y = ImGui::GetCursorScreenPos().y + (d_pplot_size / 2);

                    point_x += az_el_to_plot_x(d_pplot_size, radius, ephem.az, ephem.el);
                    point_y -= az_el_to_plot_y(d_pplot_size, radius, ephem.az, ephem.el);

                    uint8_t color[3];
                    satdump::hsv_to_rgb(fmod(ephem.scid, 10) / 10.0, 1, 1, color);
                    draw_list->AddCircleFilled({point_x, point_y}, 2 * ui_scale, ImColor(color[0], color[1], color[2], 255.0));
                }
            }

            for (auto &ephem : last_ephems)
            {
                // Draw the current satellite position
                if (ephem.el > 0 && ctime - ephem.time < 60)
                {
                    float point_x = ImGui::GetCursorScreenPos().x + (d_pplot_size / 2);
                    float point_y = ImGui::GetCursorScreenPos().y + (d_pplot_size / 2);

                    point_x += az_el_to_plot_x(d_pplot_size, radius, ephem.az, ephem.el);
                    point_y -= az_el_to_plot_y(d_pplot_size, radius, ephem.az, ephem.el);

                    draw_list->AddCircleFilled({point_x, point_y}, 5 * ui_scale, style::theme.red);
                }
            }
            all_ephem_points_mtx.unlock();

            ImGui::Dummy({(float)d_pplot_size, (float)d_pplot_size});
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        if (ImGui::BeginTable("##orbcommsatellitestable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("SCID");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Az");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("El");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("Last Epehem Age");

            all_ephem_points_mtx.lock();

            for (auto &ephem : last_ephems)
            {
                if (ephem.el > 0)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (ctime - ephem.time < 60)
                    {
                        uint8_t color[3];
                        satdump::hsv_to_rgb(fmod(ephem.scid, 10) / 10.0, 1, 1, color);
                        ImGui::TextColored(ImColor(color[0], color[1], color[2], 255.0), "%d", ephem.scid);
                    }
                    else
                    {
                        ImGui::Text("%d", ephem.scid);
                    }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.2f", ephem.az);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.2f", ephem.el);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text(PRIu64 " s", (uint64_t)(ctime - ephem.time));
                }
            }
            all_ephem_points_mtx.unlock();

            ImGui::EndTable();
        }
        ImGui::EndGroup();

        ImGui::End();
    }

    std::string OrbcommPlotterModule::getID()
    {
        return "orbcomm_plotter";
    }

    std::vector<std::string> OrbcommPlotterModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> OrbcommPlotterModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<OrbcommPlotterModule>(input_file, output_file_hint, parameters);
    }
}