#include "rtlsdr_dev.h"
#include "rtlsdr_android.h"
#include "common/dsp/complex.h"
#include "nlohmann/json.hpp"
#include <logger.h>
#include <rtl-sdr.h>

namespace satdump
{
    namespace ndsp
    {
        RTLSDRDevBlock::RTLSDRDevBlock() : DeviceBlock("rtlsdr_dev_cc", {}, {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DSPStream>(16); // TODOREWORK
        }

        RTLSDRDevBlock::~RTLSDRDevBlock()
        {
            stop(true);
        }

        void RTLSDRDevBlock::set_frequency()
        {
            std::lock_guard<std::recursive_mutex> lock(device_mutex);
            if (is_open)
            {
                int attempts = 0;
                while (attempts < 20 && rtlsdr_set_center_freq(rtlsdr_dev_obj, p_frequency) < 0)
                    attempts++;
                if (attempts == 20)
                    logger->warn("Unable to set RTL-SDR frequency!");
                else if (attempts == 0)
                {
                    logger->debug("Set RTL-SDR frequency to %d", p_frequency);

                    // This is a dirty patch! PLL might not lock on the first attempt if the frequency
                    // is above ~1 GHz, tuning down then back up locks it! This is a librtlsdr bug,
                    // because the return code should show that it wasn't successful, but it doesn't.
                    if (p_frequency > 1e9)
                    {
                        rtlsdr_set_center_freq(rtlsdr_dev_obj, p_frequency - 1e9);
                        rtlsdr_set_center_freq(rtlsdr_dev_obj, p_frequency);
                        logger->debug("Frequency was above 1 GHz, retuning to lock the PLL");
                    }
                }
                else
                    logger->debug("Set RTL-SDR frequency to %d (%d attempts!)", p_frequency, attempts + 1);
            }
        }

        void RTLSDRDevBlock::set_gains()
        {
            std::lock_guard<std::recursive_mutex> lock(device_mutex);
            if (is_open)
            {
                int attempts;
                if (changed_agc)
                {
                    // AGC Mode
                    attempts = 0;
                    while (attempts < 20 && rtlsdr_set_agc_mode(rtlsdr_dev_obj, lna_agc_enabled) < 0)
                        attempts++;
                    if (attempts == 20)
                        logger->warn("Unable to set RTL-SDR AGC mode!");
                    else if (attempts == 0)
                        logger->debug("Set RTL-SDR AGC to %d", (int)lna_agc_enabled);
                    else
                        logger->debug("Set RTL-SDR AGC to %d (%d attempts!)", (int)lna_agc_enabled, attempts + 1);

                    // Tuner gain mode
                    int tuner_gain_mode = tuner_agc_enabled ? 0 : 1;
                    attempts = 0;
                    while (attempts < 20 && rtlsdr_set_tuner_gain_mode(rtlsdr_dev_obj, tuner_gain_mode) < 0)
                        attempts++;
                    if (attempts == 20)
                        logger->warn("Unable to set RTL-SDR Tuner gain mode!");
                    else if (attempts == 0)
                        logger->debug("Set RTL-SDR Tuner gain mode to %d", tuner_gain_mode);
                    else
                        logger->debug("Set RTL-SDR Tuner gain mode to %d (%d attempts!)", tuner_gain_mode, attempts + 1);
                }

                // Get nearest supported tuner gain
                auto gain_iterator = std::lower_bound(available_gains.begin(), available_gains.end(), set_gain);
                if (gain_iterator == available_gains.end())
                    gain_iterator--;

                bool force_gain = changed_agc && !tuner_agc_enabled;
                if (changed_agc)
                    changed_agc = false;

                if (tuner_agc_enabled || (!force_gain && *gain_iterator == gain))
                    return;

                // if (gain_iterator == available_gains.begin())
                //     gain_step = 1.0f;
                // else
                //     gain_step = (float)(*gain_iterator - *std::prev(gain_iterator)) / 10.0f;

                // Set tuner gain
                attempts = 0;
                gain = *gain_iterator;
                while (attempts < 20 && rtlsdr_set_tuner_gain(rtlsdr_dev_obj, gain) < 0)
                    attempts++;
                if (attempts == 20)
                    logger->warn("Unable to set RTL-SDR Gain!");
                else if (attempts == 0)
                    logger->debug("Set RTL-SDR Gain to %.1f", (float)gain / 10.0f);
                else
                    logger->debug("Set RTL-SDR Gain to %f (%d attempts!)", (float)gain / 10.0f, attempts + 1);
            }
        }

        void RTLSDRDevBlock::set_bias()
        {
            std::lock_guard<std::recursive_mutex> lock(device_mutex);
            if (is_open)
            {
                int attempts = 0;
                while (attempts < 20 && rtlsdr_set_bias_tee(rtlsdr_dev_obj, bias_enabled) < 0)
                    attempts++;
                if (attempts == 20)
                    logger->warn("Unable to set RTL-SDR Bias!");
                else if (attempts == 0)
                    logger->debug("Set RTL-SDR Bias to %d", (int)bias_enabled);
                else
                    logger->debug("Set RTL-SDR Bias to %d (%d attempts!)", (int)bias_enabled, attempts + 1);
            }
        }

        void RTLSDRDevBlock::set_ppm()
        {
            std::lock_guard<std::recursive_mutex> lock(device_mutex);
            if (is_open && ppm_val != last_ppm)
            {
                last_ppm = ppm_val;
                int attempts = 0;
                while (attempts < 20 && rtlsdr_set_freq_correction(rtlsdr_dev_obj, ppm_val) < 0)
                    attempts++;
                if (attempts == 20)
                    logger->warn("Unable to set RTL-SDR PPM Correction!");
                else if (attempts == 0)
                    logger->debug("Set RTL-SDR PPM Correction to %d", ppm_val);
                else
                    logger->debug("Set RTL-SDR PPM Correction to %d (%d attempts!)", ppm_val, attempts + 1);
            }
        }

        void RTLSDRDevBlock::init()
        {
            set_frequency();
            set_gains();
            set_ppm();
            set_bias();
        }

        void RTLSDRDevBlock::close_device()
        {
            if (rtlsdr_dev_obj)
            {
                rtlsdr_set_bias_tee(rtlsdr_dev_obj, false);
                rtlsdr_close(rtlsdr_dev_obj);
                rtlsdr_dev_obj = nullptr;
            }
            is_open = false;
            is_started = false;
        }

        void RTLSDRDevBlock::mainThread()
        {
            const int buffer_size = 8192;
            logger->trace("RTL-SDR Buffer size %d", buffer_size);
            if (thread_should_run)
            {
                int rc = rtlsdr_read_async(rtlsdr_dev_obj, _rx_callback, this, 0, buffer_size);
                if (thread_should_run)
                    logger->error("RTL-SDR reception ended (USB disconnected or read result %d)", rc);
            }
            thread_should_run = false;
            {
                std::lock_guard<std::recursive_mutex> lock(device_mutex);
                close_device();
            }
            // A disconnected source must terminate its consumers as well. The
            // worker is the sole producer of the terminator, including on Stop.
            outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
            thread_exited = true;
        }

        void RTLSDRDevBlock::start()
        {
            const auto generation = stop_generation.load();
            std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex);
            if (is_started) return;
            if (work_thread.joinable()) work_thread.join();
            auto cancelled = [this, generation] { return stop_generation.load() != generation; };
            auto check_cancelled = [&] {
                if (cancelled()) throw satdump_exception("RTL-SDR start was cancelled");
            };
            check_cancelled();
            std::string serial;
            {
                std::lock_guard<std::recursive_mutex> lock(device_mutex);
                serial = p_serial;
            }
#ifdef __ANDROID__
            // Flowgraph::run calls Start on its worker. Stop invalidates this
            // operation through permission, opening, configuration and startup.
            rtl_android::wait_permission(serial, cancelled);
#endif
            std::lock_guard<std::recursive_mutex> device_lock(device_mutex);
            check_cancelled();
            try
            {
#ifdef __ANDROID__
                int rc = rtlsdr_open_path(&rtlsdr_dev_obj, serial.c_str());
#else
                int index = rtlsdr_get_index_by_serial(serial.c_str());
                if (index < 0)
                    throw satdump_exception("RTL-SDR dongle was not found (lookup code " + std::to_string(index) + ")");
                int rc = rtlsdr_open(&rtlsdr_dev_obj, index);
#endif
                if (rc != 0)
                {
                    std::string reason = rc == -3 ? "USB permission unavailable" : rc == -6 ? "USB device is busy" : "USB open failed";
                    throw satdump_exception("RTL-SDR: " + reason + " (rtlsdr_open code " + std::to_string(rc) + ")");
                }
                is_open = true;
                check_cancelled();
                logger->info("Opened RTL-SDR device! Serial : " + serial);

                int gains[256];
                int num_gains = rtlsdr_get_tuner_gains(rtlsdr_dev_obj, gains);
                if (num_gains > 0)
                {
                    available_gains.assign(gains, gains + num_gains);
                    std::sort(available_gains.begin(), available_gains.end());
                }

                rc = rtlsdr_set_sample_rate(rtlsdr_dev_obj, p_samplerate);
                if (rc != 0)
                    throw satdump_exception("RTL-SDR samplerate configuration failed (code " + std::to_string(rc) + ")");
                logger->debug("Set RTL-SDR samplerate to %d", p_samplerate);
                init();
                rc = rtlsdr_reset_buffer(rtlsdr_dev_obj);
                if (rc != 0)
                    throw satdump_exception("RTL-SDR USB buffer reset failed (code " + std::to_string(rc) + ")");
                check_cancelled();
                thread_should_run = true;
                thread_exited = false;
                is_started = true;
                work_thread = std::thread(&RTLSDRDevBlock::mainThread, this);
            }
            catch (...)
            {
                thread_should_run = false;
                thread_exited = true;
                close_device();
                throw;
            }
        }

        void RTLSDRDevBlock::stop(bool stop_now, bool force)
        {
            if (!stop_now) return;
            // Publish cancellation before waiting for an in-progress open.
            ++stop_generation;
            std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex);
            thread_should_run = false;
            while (!thread_exited)
            {
                {
                    std::lock_guard<std::recursive_mutex> lock(device_mutex);
                    if (rtlsdr_dev_obj) rtlsdr_cancel_async(rtlsdr_dev_obj);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            if (work_thread.joinable()) work_thread.join();
        }

        void RTLSDRDevBlock::_rx_callback(unsigned char *buf, uint32_t len, void *ctx)
        {
            RTLSDRDevBlock *tthis = ((RTLSDRDevBlock *)ctx);

            {
                int nsam = len / 2;
                DSPBuffer oblk = tthis->outputs[0].fifo->newBufferSamples(nsam, sizeof(complex_t));

                for (int i = 0; i < (int)len / 2; i++)
                {
                    oblk.getSamples<complex_t>()[i].real = (buf[i * 2 + 0] - 127.4f) / 128.0f;
                    oblk.getSamples<complex_t>()[i].imag = (buf[i * 2 + 1] - 127.4f) / 128.0f;
                }

                oblk.size = nsam;
                tthis->outputs[0].fifo->wait_enqueue(oblk);
            }
        }

        std::vector<DeviceInfo> RTLSDRDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

#ifdef __ANDROID__
            for (const auto &device : rtl_android::devices())
                r.push_back({"rtlsdr", device.name, {{"serial", device.path}}, nlohmann::ordered_json::object()});
#else
            int c = rtlsdr_get_device_count();

            for (int i = 0; i < c; i++)
            {
                const char *name = rtlsdr_get_device_name(i);
                char manufact[256]{}, product[256]{}, serial[256]{};
                std::string _name;
                if (rtlsdr_get_device_usb_strings(i, manufact, product, serial) != 0)
                    _name = std::string(name) + " #" + std::to_string(i), std::to_string(i); //   results.push_back({"rtlsdr", std::string(name) + " #" + std::to_string(i), std::to_string(i)});
                else
                    _name = std::string(manufact) + " " + std::string(product) + " #" + std::string(serial),
                    std::string(serial); //    results.push_back({"rtlsdr", std::string(manufact) + " " + std::string(product) + " #" + std::string(serial), std::string(serial)});

                nlohmann::ordered_json _c;

                rtlsdr_dev *rtlsdr_dev_obj;
                int index = rtlsdr_get_index_by_serial(serial);
                if (index >= 0 && rtlsdr_open(&rtlsdr_dev_obj, index) == 0)
                {
                    // Get available gains
                    int gains[256];
                    int num_gains = rtlsdr_get_tuner_gains(rtlsdr_dev_obj, gains);
                    if (num_gains > 0)
                        add_param_range(_c, "gain", "float", gains[0] / 10.0f, gains[num_gains - 1] / 10.0f, 0.1);

                    rtlsdr_close(rtlsdr_dev_obj);
                }
                else
                {
                    logger->warn("Could not probe RTL-SDR device. Not providing specific parameters!");
                }

                nlohmann::json p;
                p["serial"] = std::string(serial);
                r.push_back({"rtlsdr", _name, p, _c});
            }

#endif
            return r;
        }
    } // namespace ndsp
} // namespace satdump
