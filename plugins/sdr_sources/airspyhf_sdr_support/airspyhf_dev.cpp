#include "airspyhf_dev.h"
#include "common/dsp/complex.h"
#include "core/exception.h"
#include <logger.h>

namespace satdump
{
    namespace ndsp
    {
        AirspyHFDevBlock::AirspyHFDevBlock() : DeviceBlock("airspyhf_dev_cc", {}, {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DSPStream>(16); // TODOREWORK
        }

        AirspyHFDevBlock::~AirspyHFDevBlock()
        {
            stop(); // TODOREWORK (make sure to close device)
        }

        void AirspyHFDevBlock::set_frequency()
        {
            if (is_open)
            {
                airspyhf_set_freq(airspyhf_dev_obj, p_frequency);
                logger->debug("Set AirspyHF frequency to %f", p_frequency);
            }
        }

        void AirspyHFDevBlock::set_gains()
        {
            if (is_open)
            {
                airspyhf_set_hf_lna(airspyhf_dev_obj, hf_lna_enabled);
                logger->debug("Set AirspyHF HF LNA to %d", (int)hf_lna_enabled);

                airspyhf_set_hf_att(airspyhf_dev_obj, attenuation / 6.0f);
                logger->debug("Set AirspyHF HF Attentuation to %d", attenuation);
            }
        }

        void AirspyHFDevBlock::set_others()
        {
            if (is_open)
            {
                int agc_mode = 0;
                if (this->agc_mode == "off")
                    agc_mode = 0;
                else if (this->agc_mode == "low")
                    agc_mode = 1;
                else if (this->agc_mode == "high")
                    agc_mode = 2;
                else
                    throw satdump_exception("Invalid AirspyHF AGC Mode!");

                airspyhf_set_hf_agc(airspyhf_dev_obj, agc_mode != 0);
                airspyhf_set_hf_agc_threshold(airspyhf_dev_obj, agc_mode - 1);
                logger->debug("Set AirspyHF HF AGC Mode to %d", (int)agc_mode);
            }
        }

        void AirspyHFDevBlock::init()
        {
            set_frequency();
            set_gains();
            set_others();
        }

        void AirspyHFDevBlock::start()
        {
            if (airspyhf_open_sn(&airspyhf_dev_obj, std::stoull(p_serial)) != AIRSPYHF_SUCCESS)
                throw satdump_exception("Could not open AirspyHF device! Serial : " + p_serial);
            logger->info("Opened AirspyHF device! Serial : " + p_serial);
            is_open = true;

            airspyhf_set_samplerate(airspyhf_dev_obj, p_samplerate);
            logger->debug("Set AirspyHF samplerate to %d", p_samplerate);

            init();

            airspyhf_start(airspyhf_dev_obj, &_rx_callback, this);
            is_started = true;
        }

        void AirspyHFDevBlock::stop(bool stop_now, bool force)
        {
            if (stop_now && is_started) // TODOREWORK Split wait & stop?
            {
                airspyhf_stop(airspyhf_dev_obj);
                airspyhf_close(airspyhf_dev_obj);
                is_started = false;
                is_open = false;
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
            }
        }

        int AirspyHFDevBlock::_rx_callback(airspyhf_transfer_t *t)
        {
            AirspyHFDevBlock *tthis = (AirspyHFDevBlock *)t->ctx;
            int &size = t->sample_count;

            DSPBuffer oblk = tthis->outputs[0].fifo->newBufferSamples(size, sizeof(complex_t));
            memcpy(oblk.getSamples<complex_t>(), t->samples, size * sizeof(complex_t));
            oblk.size = size;
            tthis->outputs[0].fifo->wait_enqueue(oblk);

            return 0;
        }

        std::vector<DeviceInfo> AirspyHFDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

            uint64_t serials[100];
            int c = airspyhf_list_devices(serials, 100);

            for (int i = 0; i < c; i++)
            {
                std::stringstream ss;
                ss << std::hex << serials[i];
                std::string name = "AirSpyHF " + ss.str();

                nlohmann::json c;

                airspyhf_device *airspy_dev_obj;
                if (airspyhf_open_sn(&airspy_dev_obj, serials[i]) == AIRSPYHF_SUCCESS)
                {
                    // Get a better name
                    char version[255 + 1];
                    airspyhf_version_string_read(airspy_dev_obj, version, 255);
                    //  if (std::string(version).find("MINI") != std::string::npos)
                    //      name = "AirSpy Mini " + ss.str();
                    logger->critical(std::string(version));

                    // Get available samplerates
                    uint32_t samprate_cnt;
                    uint32_t dev_samplerates[10];
                    airspyhf_get_samplerates(airspy_dev_obj, &samprate_cnt, 0);
                    airspyhf_get_samplerates(airspy_dev_obj, dev_samplerates, samprate_cnt);
                    c["samplerate"]["type"] = "samplerate";

                    for (int i = samprate_cnt - 1, i2 = 0; i >= 0; i--, i2++)
                        c["samplerate"]["list"][i2] = dev_samplerates[i];

                    airspyhf_close(airspy_dev_obj);
                }
                else
                {
                    logger->warn("Could not probe AirspyHF device. Not providing specific parameters!");
                }

                nlohmann::json p;
                p["serial"] = std::to_string(serials[i]);
                r.push_back({"airspyhf", name, p, c});
            }

            return r;
        }
    } // namespace ndsp
} // namespace satdump