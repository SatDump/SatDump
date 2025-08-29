#include "airspy_dev.h"
#include "common/dsp/complex.h"
#include <logger.h>

namespace satdump
{
    namespace ndsp
    {
        AirspyDevBlock::AirspyDevBlock() : DeviceBlock("airspy_dev_cc", {}, {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DSPStream>(16); // TODOREWORK
        }

        AirspyDevBlock::~AirspyDevBlock()
        {
            stop(); // TODOREWORK (make sure to close device)
        }

        void AirspyDevBlock::set_frequency()
        {
            if (is_open)
            {
                airspy_set_freq(airspy_dev_obj, p_frequency);
                logger->debug("Set Airspy frequency to %f", p_frequency);
            }
        }

        void AirspyDevBlock::set_gains()
        {
            if (is_open)
            {
                if (p_gain_type == "sensitive")
                {
                    airspy_set_sensitivity_gain(airspy_dev_obj, p_gain);
                    logger->debug("Set Airspy gain (sensitive) to %d", p_gain);
                }
                else if (p_gain_type == "linear")
                {
                    airspy_set_linearity_gain(airspy_dev_obj, p_gain);
                    logger->debug("Set Airspy gain (linear) to %d", p_gain);
                }
                else if (p_gain_type == "manual")
                {
                    airspy_set_lna_gain(airspy_dev_obj, p_lna_gain);
                    airspy_set_mixer_gain(airspy_dev_obj, p_mixer_gain);
                    airspy_set_vga_gain(airspy_dev_obj, p_vga_gain);
                    logger->debug("Set Airspy gain (manual) to %d, %d, %d", p_lna_gain, p_mixer_gain, p_vga_gain);
                }
            }
        }

        void AirspyDevBlock::set_others()
        {
            if (is_open)
            {
                airspy_set_rf_bias(airspy_dev_obj, p_bias);
                logger->debug("Set Airspy bias to %d", (int)p_bias);

                airspy_set_lna_agc(airspy_dev_obj, p_lna_agc);
                airspy_set_mixer_agc(airspy_dev_obj, p_mixer_agc);
                logger->debug("Set Airspy LNA AGC to %d", p_lna_agc);
                logger->debug("Set Airspy Mixer AGC to %d", p_mixer_agc);
            }
        }

        void AirspyDevBlock::init()
        {
            set_frequency();
            set_gains();
            set_others();
        }

        void AirspyDevBlock::start()
        {
            if (airspy_open_sn(&airspy_dev_obj, std::stoull(p_serial)) != AIRSPY_SUCCESS)
                throw satdump_exception("Could not open Airspy One device! Serial : " + p_serial);
            logger->info("Opened Airspy One device! Serial : " + p_serial);
            is_open = true;

            airspy_set_samplerate(airspy_dev_obj, p_samplerate);
            logger->debug("Set Airspy samplerate to %d", p_samplerate);

            airspy_set_packing(airspy_dev_obj, 1);
            airspy_set_sample_type(airspy_dev_obj, AIRSPY_SAMPLE_FLOAT32_IQ);

            init();

            airspy_start_rx(airspy_dev_obj, &_rx_callback, this);
            is_started = true;
        }

        void AirspyDevBlock::stop(bool stop_now)
        {
            if (stop_now && is_started) // TODOREWORK Split wait & stop?
            {
                airspy_set_rf_bias(airspy_dev_obj, false);
                airspy_stop_rx(airspy_dev_obj);
                airspy_close(airspy_dev_obj);
                is_started = false;
                is_open = false;
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
            }
        }

        int AirspyDevBlock::_rx_callback(airspy_transfer *t)
        {
            AirspyDevBlock *tthis = (AirspyDevBlock *)t->ctx;
            int &size = t->sample_count;

            if (t->sample_type == AIRSPY_SAMPLE_FLOAT32_IQ)
            {
                DSPBuffer oblk = tthis->outputs[0].fifo->newBufferSamples<complex_t>(size);
                memcpy(oblk.getSamples<complex_t>(), t->samples, size * sizeof(complex_t));
                oblk.size = size;
                tthis->outputs[0].fifo->wait_enqueue(oblk);
            }
            else
            {
                logger->error("TODO IMPLEMENT SAMPLE TYPE %d\n", t->sample_type);
            }

            return 0;
        }

        std::vector<DeviceInfo> AirspyDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

            uint64_t serials[100];
            int c = airspy_list_devices(serials, 100);

            for (int i = 0; i < c; i++)
            {
                std::stringstream ss;
                ss << std::hex << serials[i];
                std::string name = "AirSpy One " + ss.str();

                nlohmann::json c;
                nlohmann::json p;

                airspy_device *airspy_dev_obj;
                if (airspy_open_sn(&airspy_dev_obj, serials[i]) == AIRSPY_SUCCESS)
                {
                    // Get a better name
                    char version[255 + 1];
                    airspy_version_string_read(airspy_dev_obj, version, 255);
                    if (std::string(version).find("MINI") != std::string::npos)
                        name = "AirSpy Mini " + ss.str();

                    // Get available samplerates
                    uint32_t samprate_cnt;
                    uint32_t dev_samplerates[10];
                    airspy_get_samplerates(airspy_dev_obj, &samprate_cnt, 0);
                    airspy_get_samplerates(airspy_dev_obj, dev_samplerates, samprate_cnt);
                    c["samplerate"]["type"] = "samplerate";

                    bool has_10msps = false;
                    for (int i = samprate_cnt - 1, i2 = 0; i >= 0; i--, i2++)
                    {
                        c["samplerate"]["list"][i2] = dev_samplerates[i];
                        if (dev_samplerates[i] == 10e6)
                            has_10msps = true;
                    }

                    if (!has_10msps)
                        c["samplerate"]["list"].push_back(10e6);

                    p["samplerate"] = dev_samplerates[0];

                    airspy_close(airspy_dev_obj);
                }
                else
                {
                    logger->warn("Could not probe Airspy device. Not providing specific parameters!");
                }

                p["serial"] = std::to_string(serials[i]);
                r.push_back({"airspy", name, p, c});
            }

            return r;
        }
    } // namespace ndsp
} // namespace satdump