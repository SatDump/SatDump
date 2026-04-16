#pragma once

#include "common/dsp/complex.h"
#include "common/dsp/filter/firdes.h"
#include "dsp/agc/agc.h"
#include "dsp/block.h"
#include "dsp/conv/complex_to_imag.h"
#include "dsp/conv/complex_to_mag.h"
#include "dsp/conv/complex_to_real.h"
#include "dsp/filter/fir.h"
#include "dsp/path/selector.h"
#include "dsp/path/switch.h"
#include "dsp/resampling/rational_resampler.h"
#include "dsp/utils/quadrature_demod.h"
#include "nlohmann/json.hpp"
#include <string>

namespace satdump
{
    namespace ndsp
    {
        class AudioDemodHierBlock : public Block
        {
        private:
            SwitchBlock<complex_t> main_switch;

            AGCBlock<complex_t> nfm_agc;
            QuadratureDemodBlock nfm_qua;
            RationalResamplerBlock<float> nfm_res;

            AGCBlock<complex_t> am_agc;
            ComplexToMagBlock am_mag;
            RationalResamplerBlock<float> am_res;

            FIRBlock<complex_t, complex_t> usb_flt;
            AGCBlock<complex_t> usb_agc;
            ComplexToRealBlock usb_mag;
            RationalResamplerBlock<float> usb_res;

            FIRBlock<complex_t, complex_t> lsb_flt;
            AGCBlock<complex_t> lsb_agc;
            ComplexToImagBlock lsb_mag;
            RationalResamplerBlock<float> lsb_res;

            SelectorBlock<float> main_selector;

        private:
            bool running = false;

            std::string mode = "nfm";
            double samplerate = 6e6;
            double audio_samplerate = 48e3;

        public:
            AudioDemodHierBlock();
            ~AudioDemodHierBlock();

            std::vector<BlockIO> get_inputs() { return main_switch.get_inputs(); }
            std::vector<BlockIO> get_outputs() { return main_selector.get_outputs(); }
            void set_input(BlockIO f, int i) { main_switch.set_input(f, i); }
            BlockIO get_output(int i, int nbuf) { return main_selector.get_output(i, nbuf); }

            void init()
            {
                main_switch.set_cfg("noutputs", 4);
                main_switch.set_cfg("active_id", 0);

                main_selector.set_cfg("ninputs", 4);
                main_selector.set_cfg("active_id", 0);

                set_cfg("mode", mode);

                nfm_agc.set_input(main_switch.get_output(0, 4), 0);
                nfm_qua.link(&nfm_agc, 0, 0, 4);
                nfm_res.link(&nfm_qua, 0, 0, 4);

                nfm_qua.set_cfg("gain", 20);
                nfm_res.set_cfg("decimation", samplerate);
                nfm_res.set_cfg("interpolation", audio_samplerate);

                am_agc.set_input(main_switch.get_output(1, 4), 0);
                am_mag.link(&am_agc, 0, 0, 4);
                am_res.link(&am_mag, 0, 0, 4);

                am_agc.set_cfg("reference", 0.1);
                am_agc.set_cfg("rate", 0.05);
                am_res.set_cfg("decimation", samplerate);
                am_res.set_cfg("interpolation", audio_samplerate);

                usb_flt.set_input(main_switch.get_output(2, 4), 0);
                usb_agc.link(&usb_flt, 0, 0, 4);
                usb_mag.link(&usb_agc, 0, 0, 4);
                usb_res.link(&usb_mag, 0, 0, 4);

                usb_flt.set_cfg("taps", dsp::firdes::complex_band_pass(1, samplerate, 0, samplerate / 2., 100));
                usb_agc.set_cfg("reference", 0.2);
                usb_agc.set_cfg("rate", 0.1);
                usb_res.set_cfg("decimation", samplerate);
                usb_res.set_cfg("interpolation", audio_samplerate);

                lsb_flt.set_input(main_switch.get_output(3, 4), 0);
                lsb_agc.link(&lsb_flt, 0, 0, 4);
                lsb_mag.link(&lsb_agc, 0, 0, 4);
                lsb_res.link(&lsb_mag, 0, 0, 4);

                lsb_flt.set_cfg("taps", dsp::firdes::complex_band_pass(1, samplerate, -samplerate / 2., 0, 100));
                lsb_agc.set_cfg("reference", 0.2);
                lsb_agc.set_cfg("rate", 0.1);
                lsb_res.set_cfg("decimation", samplerate);
                lsb_res.set_cfg("interpolation", audio_samplerate);

                main_selector.link(&nfm_res, 0, 0, 4);
                main_selector.link(&am_res, 0, 1, 4);
                main_selector.link(&usb_res, 0, 2, 4);
                main_selector.link(&lsb_res, 0, 3, 4);
            }

            void start()
            {
                init();

                main_switch.start();

                nfm_agc.start();
                nfm_qua.start();
                nfm_res.start();

                am_agc.start();
                am_mag.start();
                am_res.start();

                usb_flt.start();
                usb_agc.start();
                usb_mag.start();
                usb_res.start();

                lsb_flt.start();
                lsb_agc.start();
                lsb_mag.start();
                lsb_res.start();

                main_selector.start();

                running = true;
            }

            void stop(bool stop_snow, bool force)
            {
                main_switch.stop(stop_snow);

                nfm_agc.stop(stop_snow);
                nfm_qua.stop(stop_snow);
                nfm_res.stop(stop_snow);

                am_agc.stop(stop_snow);
                am_mag.stop(stop_snow);
                am_res.stop(stop_snow);

                usb_flt.stop(stop_snow);
                usb_agc.stop(stop_snow);
                usb_mag.stop(stop_snow);
                usb_res.stop(stop_snow);

                lsb_flt.stop(stop_snow);
                lsb_agc.stop(stop_snow);
                lsb_mag.stop(stop_snow);
                lsb_res.stop(stop_snow);

                main_selector.stop(stop_snow);

                running = false;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json v;

                add_param_simple(v, "samplerate", "float");
                add_param_simple(v, "audio_samplerate", "float");

                add_param_list(v, "mode", "string", {"nfm", "am", "usb", "lsb"});

                return v;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samplerate")
                    return samplerate;
                else if (key == "audio_samplerate")
                    return audio_samplerate;
                else if (key == "mode")
                    return mode;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samplerate" || key == "audio_samplerate")
                {
                    if (key == "samplerate")
                        samplerate = v;
                    if (key == "audio_samplerate")
                        audio_samplerate = v;

                    cfg_res_t res = RES_OK;

                    res = std::max(nfm_res.set_cfg("decimation", samplerate), res);
                    res = std::max(nfm_res.set_cfg("interpolation", audio_samplerate), res);

                    res = std::max(am_res.set_cfg("decimation", samplerate), res);
                    res = std::max(am_res.set_cfg("interpolation", audio_samplerate), res);

                    res = std::max(usb_res.set_cfg("decimation", samplerate), res);
                    res = std::max(usb_res.set_cfg("interpolation", audio_samplerate), res);

                    res = std::max(lsb_res.set_cfg("decimation", samplerate), res);
                    res = std::max(lsb_res.set_cfg("interpolation", audio_samplerate), res);

                    return res;
                }
                else if (key == "mode" && (v == "nfm" || v == "am" || v == "usb" || v == "lsb"))
                {
                    mode = v;

                    if (v == "nfm")
                    {
                        main_switch.set_cfg("active_id", 0);
                        main_selector.set_cfg("active_id", 0);
                    }
                    else if (v == "am")
                    {
                        main_switch.set_cfg("active_id", 1);
                        main_selector.set_cfg("active_id", 1);
                    }
                    else if (v == "usb")
                    {
                        main_switch.set_cfg("active_id", 2);
                        main_selector.set_cfg("active_id", 2);
                    }
                    else if (v == "lsb")
                    {
                        main_switch.set_cfg("active_id", 3);
                        main_selector.set_cfg("active_id", 3);
                    }

                    return RES_OK;
                }
                else
                {
                    return RES_ERR;
                }
            }

            bool work() { return false; }
        };
    } // namespace ndsp
} // namespace satdump