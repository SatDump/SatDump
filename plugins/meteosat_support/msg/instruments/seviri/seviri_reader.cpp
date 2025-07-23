#include "seviri_reader.h"
#include "../../msg.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "logger.h"
#include "products/image_product.h"
#include "utils/stats.h"
#include <cmath>
#include <filesystem>

#include "common/tracking/tle.h"
#include "core/config.h"
#include "utils/thread_priority.h"
// TODOREWORK BRING BACK #include "products/processor/image_processor.h"

namespace lrit
{
    std::string timestamp_to_string2(double timestamp);
}

namespace meteosat
{
    namespace msg
    {
        double parseCCSDSTimeMeteosat(ccsds::CCSDSPacket &pkt, int offset, int ms_scale, double us_of_ms_scale)
        {
            uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
            uint32_t milliseconds_of_day = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];
            uint16_t microseconds_of_millisecond = pkt.payload[6] << 8 | pkt.payload[7];

            return double(offset) * 86400.0 + (days * 18.204444444 * 3600.0) + double(milliseconds_of_day) / double(ms_scale) + double(microseconds_of_millisecond) / us_of_ms_scale;
        }

        SEVIRIReader::SEVIRIReader(bool d_mode_is_rss) : d_mode_is_rss(d_mode_is_rss)
        {
            // Standard resolution channels
            for (int i = 0; i < 11; i++)
            {
                images_nrm[i] = image::Image(16, 3834, d_mode_is_rss ? 1494 : 4482, 1);
                images_nrm[i].fill(0);
            }

            // Timestamps
            timestamps_nrm.resize(d_mode_is_rss ? 1494 : 4482, -1);

            // HRV channel
            images_hrv = image::Image(16, 5751, d_mode_is_rss ? 4500 : 13500, 1);
            images_hrv.fill(0);

            lines_since_last_end = 0;
            not_channels_lines = 0;

            // Automatic composite generation
            if (satdump::satdump_cfg.shouldAutoprocessProducts())
                can_make_composites = true;

            if (can_make_composites)
                compositeGeneratorThread = std::thread(&SEVIRIReader::compositeThreadFunc, this);
        }

        SEVIRIReader::~SEVIRIReader()
        {
            if (textureID > 0)
                delete[] textureBuffer;

            if (can_make_composites)
            {
                int queue_size;
                do
                {
                    compo_queue_mtx.lock();
                    queue_size = compo_queue.size();
                    compo_queue_mtx.unlock();
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                } while (queue_size > 0);

                composite_th_should_run = false;
                if (compositeGeneratorThread.joinable())
                    compositeGeneratorThread.join();
            }
        }

        void SEVIRIReader::saveImages()
        {
            is_saving = true;

            std::string directory = d_directory + "/" + lrit::timestamp_to_string2(last_timestamp) + "/";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directories(directory);

            satdump::products::ImageProduct *seviri_products = new satdump::products::ImageProduct();

            seviri_products->set_product_timestamp(last_timestamp);

            int scid = satdump::most_common(all_scids.begin(), all_scids.end(), 0);

            if (scid == METEOSAT_8_SCID)
                seviri_products->set_product_source("MSG-1");
            else if (scid == METEOSAT_9_SCID)
                seviri_products->set_product_source("MSG-2");
            else if (scid == METEOSAT_10_SCID)
                seviri_products->set_product_source("MSG-3");
            else if (scid == METEOSAT_11_SCID)
                seviri_products->set_product_source("MSG-4");

            int norad = -1;

            if (scid == METEOSAT_8_SCID)
                norad = METEOSAT_8_NORAD;
            else if (scid == METEOSAT_9_SCID)
                norad = METEOSAT_9_NORAD;
            else if (scid == METEOSAT_10_SCID)
                norad = METEOSAT_10_NORAD;
            else if (scid == METEOSAT_11_SCID)
                norad = METEOSAT_11_NORAD;

            std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, last_timestamp);

            seviri_products->instrument_name = "seviri";
            //           seviri_products->set_timestamps(timestamps_nrm);
            //            seviri_products->set_tle(satellite_tle); TODOREWORK? Projection!

            int ch_offsets[12] = {
                0, -18, 18,
                0, // TODO
                0, // TODO
                0, // TODO
                0, // TODO
                0, // TODO
                0, // TODO
                0, // TODO
                0, // TODO
                0, // TODO, HRV WILL BE HARDER!
            };

            for (int i = 0; i < 11; i++)
            {
                images_nrm[i].mirror(true, true);
                seviri_products->images.push_back(
                    {i, "SEVIRI-" + std::to_string(i + 1), std::to_string(i + 1), images_nrm[i], 10, satdump::ChannelTransform().init_affine(3, 3, ch_offsets[i] * 3, 0)});
                images_nrm[i].fill(0);
            }

            for (auto &v : timestamps_nrm)
                v = -1;

            images_hrv.mirror(true, true);
            seviri_products->images.push_back({11, "SEVIRI-12", "12", images_hrv, 10, satdump::ChannelTransform().init_affine(1, 1, ch_offsets[11], 0)});
            images_hrv.fill(0);

            seviri_products->save(directory);
            if (can_make_composites)
            {
                // We do NOT want to overload our queue and run out of RAM...
                {
                    int queue_size;
                    do
                    {
                        compo_queue_mtx.lock();
                        queue_size = compo_queue.size();
                        compo_queue_mtx.unlock();
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    } while (queue_size > 50);
                }

                compo_queue_mtx.lock();
                compo_queue.push_back({seviri_products, directory});
                compo_queue_mtx.unlock();
            }
            else
            {
                delete seviri_products;
            }

            is_saving = false;
        }

        void SEVIRIReader::compositeThreadFunc()
        {
            satdump::setLowestThreadPriority(); // TODOREWORK namespace remove
            while (composite_th_should_run)
            {
                compo_queue_mtx.lock();
                int queue_size = compo_queue.size();
                compo_queue_mtx.unlock();

                if (queue_size == 0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                compo_queue_mtx.lock();
                satdump::products::ImageProduct *pro = (satdump::products::ImageProduct *)compo_queue[0].first;
                std::string pro_path = compo_queue[0].second;
                compo_queue.erase(compo_queue.begin());
                compo_queue_mtx.unlock();

                try
                {
                    // TODOREWORK satdump::process_image_products((satdump::Products *)pro, pro_path);
                    logger->critical("PROCESSING TODOREWORK MUST BE IMPLEMENTED!!!");
                    delete pro;
                    pro = nullptr;
                }
                catch (std::exception &e)
                {
                    logger->error("Error trying to autogen MSG composites! : %s", e.what());
                    delete pro;
                }
            }
        }

        void SEVIRIReader::work(int scid, ccsds::CCSDSPacket &pkt)
        {
            int scan_chunk_number = pkt.header.packet_sequence_count % 16;
            double scan_timestamp = 0;

            if (scid == METEOSAT_9_SCID)
                scan_timestamp = parseCCSDSTimeMeteosat(pkt, 18249 + 1310, 65536, 1e100) + 6442;
            else if (scid == METEOSAT_10_SCID)
                scan_timestamp = parseCCSDSTimeMeteosat(pkt, 18249, 65536, 1e100) + 34738;
            else if (scid == METEOSAT_11_SCID)
                scan_timestamp = parseCCSDSTimeMeteosat(pkt, 18249 + 731, 65536, 1e100) + 42207;
            else
                return;

            last_timestamp = scan_timestamp;
            all_scids.push_back(scid);

            // Normal resolution channels 1 to 11
            if (scan_chunk_number < 11)
            {
                if (scan_chunk_number == 0)
                {
                    time_t tttime = scan_timestamp;
                    std::tm timeReadable = *gmtime(&tttime);

                    if (d_mode_is_rss)
                    {
                        if (lines_since_last_end++ > 500 && (timeReadable.tm_min == 0 || timeReadable.tm_min == 5 || timeReadable.tm_min == 10 || timeReadable.tm_min == 15 ||
                                                             timeReadable.tm_min == 20 || timeReadable.tm_min == 25 || timeReadable.tm_min == 30 || timeReadable.tm_min == 35 ||
                                                             timeReadable.tm_min == 40 || timeReadable.tm_min == 45 || timeReadable.tm_min == 50 || timeReadable.tm_min == 55))
                        {
                            lines_since_last_end = 0;
                            saveImages();
                        }
                    }
                    else
                    {
                        if (lines_since_last_end++ > 500 && (timeReadable.tm_min == 0 || timeReadable.tm_min == 15 || timeReadable.tm_min == 30 || timeReadable.tm_min == 45))
                        {
                            lines_since_last_end = 0;
                            saveImages();
                        }
                    }
                }

                size_t lines = d_mode_is_rss ? (fmod(scan_timestamp, 5 * 60) / (300 / 1494.0)) : (fmod(scan_timestamp, 15 * 60) / (300 / 1494.0));

                // Timestamp, somewhat interpolated to have one on all lines,
                // assuming the scan rate to be right
                if (scan_chunk_number == 0 && lines + 2 <= timestamps_nrm.size())
                {
                    timestamps_nrm[lines + 0] = scan_timestamp + 0.2 * 0;
                    timestamps_nrm[lines + 1] = scan_timestamp + 0.2 * 1;
                    timestamps_nrm[lines + 2] = scan_timestamp + 0.2 * 2;
                }

                repackBytesTo10bits(&pkt.payload[8], pkt.payload.size() - 8, tmp_linebuf_nrm);
                for (int c = 0; c < 3; c++)
                {
                    // Some channels are swapped on the focal plane
                    bool swap = scan_chunk_number == 3 || scan_chunk_number == 4 || scan_chunk_number == 8 || scan_chunk_number == 9 || scan_chunk_number == 10;
                    for (int v = 0; v < 3834; v++)
                        if (lines < images_nrm[scan_chunk_number].height())
                            images_nrm[scan_chunk_number].set(lines * images_nrm[scan_chunk_number].width() + v, tmp_linebuf_nrm[(swap ? (2 - c) : c) * 3834 + v] << 6);
                    lines++;
                }

                // If the UI is active, update texture
                if (textureID > 0 && lines % 40 == 0)
                {
                    // Downscale image
                    int img_height = 1000;
                    int img_width = 1000;
                    image::Image imageScaled = images_nrm[3].to8bits();
                    imageScaled.resize(img_width, img_height);
                    if (imageScaled.typesize() == 1)
                        image::image_to_rgba(imageScaled, textureBuffer);
                    hasToUpdate = true;
                    // logger->info("Update!!!!");
                }
            }
            else if (scan_chunk_number < 15)
            {
                uint16_t *tmp_buf = new uint16_t[15000];
                repackBytesTo10bits(&pkt.payload[8], pkt.payload.size() - 8, tmp_buf);

                size_t lines = d_mode_is_rss ? (fmod(scan_timestamp, 5 * 60) / (100 / 1494.0)) : fmod(scan_timestamp, 15 * 60) / (100 / 1494.0);
                lines += (scan_chunk_number - 11) * 2;

                if (not_channels_lines != lines)
                    logger->trace("bogus line: %d", lines - not_channels_lines);

                for (int c = 0; c < 2; c++)
                {
                    for (int v = 0; v < 5751; v++)
                        if (lines < images_hrv.height())
                            images_hrv.set(lines * images_hrv.width() + v, tmp_buf[c * 5751 + v] << 6);
                    lines++;
                    // printf("Time %s - %f  %f, lines:%d cpayload:%d\n", timestamp_to_string(ltime).c_str(), ltime, last_time, lines, cpayload);
                }
                not_channels_lines = lines;

                pkt.payload.resize(14392 - 6);
                delete[] tmp_buf;
            }
            else
            {
                uint16_t *tmp_buf = new uint16_t[15000];
                repackBytesTo10bits(&pkt.payload[8], pkt.payload.size() - 8, tmp_buf);

                size_t lines = d_mode_is_rss ? (fmod(scan_timestamp, 5 * 60) / (100 / 1494.0)) : fmod(scan_timestamp, 15 * 60) / (100 / 1494.0);
                lines += (scan_chunk_number - 11) * 2;

                for (int v = 0; v < 5751; v++)
                    if (lines < images_hrv.height())
                        images_hrv.set(lines * images_hrv.width() + v, tmp_buf[v] << 6);

                lines++;
                not_channels_lines = lines;
                delete[] tmp_buf;
            }
        }
    } // namespace msg
} // namespace meteosat