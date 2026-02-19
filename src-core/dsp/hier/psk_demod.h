#pragma once

#include "common/dsp/complex.h"
#include "dsp/agc/agc.h"
#include "dsp/block.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/filter/rrc.h"
#include "dsp/pll/costas.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "utils/string.h"
#include <string>

namespace satdump
{
    namespace ndsp
    {
        class PSKDemodHierBlock : public Block
        {
        private:
            RRC_FIRBlock<complex_t> rrc_blk;
            AGCBlock<complex_t> agc_blk;
            MMClockRecoveryBlock<complex_t> rec_blk;
            CostasBlock pll_blk;

        private:
            bool running = false;

            std::string constellation = "bpsk";
            double samplerate = 6e6;
            double symbolrate = 2e6;
            bool advanced_mode = false;

        public:
            PSKDemodHierBlock();
            ~PSKDemodHierBlock();

            std::vector<BlockIO> get_inputs() { return rrc_blk.get_inputs(); }
            std::vector<BlockIO> get_outputs() { return pll_blk.get_outputs(); }
            void set_input(BlockIO f, int i) { rrc_blk.set_input(f, i); }
            BlockIO get_output(int i, int nbuf) { return pll_blk.get_output(i, nbuf); }

            void init()
            {
                /*if (running)
                {
                    pll_blk.stop(true, true);
                    rec_blk.stop(true, true);
                    agc_blk.stop(true, true);
                }

                if (constellation == "oqpsk")
                {
                    logger->error("TODOREWORK!");
                }
                else
                {
                    agc_blk.link(&rrc_blk, 0, 0, 4);
                    rec_blk.link(&agc_blk, 0, 0, 4);
                    pll_blk.link(&rec_blk, 0, 0, 4);
                }

                if (running)
                {
                    agc_blk.start();
                    rec_blk.start();
                    pll_blk.start();
                }*/
            }

            void start()
            {
                init();

                rrc_blk.start();
                agc_blk.start();
                rec_blk.start();
                pll_blk.start();

                running = true;
            }

            void stop(bool stop_snow, bool force)
            {
                rrc_blk.stop(stop_snow);
                agc_blk.stop(stop_snow);
                rec_blk.stop(stop_snow);
                pll_blk.stop(stop_snow);

                running = false;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json v;

                add_param_list(v, "constellation", "string", {"bpsk", "qpsk"});
                add_param_simple(v, "samplerate", "float");
                add_param_simple(v, "symbolrate", "float");
                add_param_simple(v, "advanced", "bool");

                if (advanced_mode)
                {
                    auto rrc_list = rrc_blk.get_cfg_list();
                    for (auto &i : rrc_list.items())
                    {
                        if (i.value().contains("name"))
                            i.value()["name"] = "RRC " + i.value()["name"].get<std::string>();
                        v["rrc_" + i.key()] = i.value();
                    }
                    auto agc_list = agc_blk.get_cfg_list();
                    for (auto &i : agc_list.items())
                    {
                        if (i.value().contains("name"))
                            i.value()["name"] = "AGC " + i.value()["name"].get<std::string>();
                        v["agc_" + i.key()] = i.value();
                    }
                    auto rec_list = rec_blk.get_cfg_list();
                    for (auto &i : rec_list.items())
                    {
                        if (i.value().contains("name"))
                            i.value()["name"] = "Rec " + i.value()["name"].get<std::string>();
                        v["rec_" + i.key()] = i.value();
                    }
                    auto pll_list = pll_blk.get_cfg_list();
                    for (auto &i : pll_list.items())
                    {
                        if (i.value().contains("name"))
                            i.value()["name"] = "PLL " + i.value()["name"].get<std::string>();
                        v["pll_" + i.key()] = i.value();
                    }
                }

                return v;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "constellation")
                    return constellation;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "symbolrate")
                    return symbolrate;
                else if (key == "advanced")
                    return advanced_mode;
                else if (key.find("rrc_") == 0)
                {
                    replaceAllStr(key, "rrc_", "");
                    return rrc_blk.get_cfg(key);
                }
                else if (key.find("agc_") == 0)
                {
                    replaceAllStr(key, "agc_", "");
                    return agc_blk.get_cfg(key);
                }
                else if (key.find("rec_") == 0)
                {
                    replaceAllStr(key, "rec_", "");
                    return rec_blk.get_cfg(key);
                }
                else if (key.find("pll_") == 0)
                {
                    replaceAllStr(key, "pll_", "");
                    return pll_blk.get_cfg(key);
                }
                else
                {
                    return nlohmann::ordered_json();
                }
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "constellation" && (v == "bpsk" || v == "qpsk"))
                {
                    constellation = v;

                    init();

                    if (v == "bpsk")
                        return pll_blk.set_cfg("order", 2);
                    else
                        return pll_blk.set_cfg("order", 4);
                }
                else if (key == "samplerate" || key == "symbolrate")
                {
                    if (key == "samplerate")
                        samplerate = v;
                    if (key == "symbolrate")
                        symbolrate = v;

                    cfg_res_t res = RES_OK;
                    res = std::max(rrc_blk.set_cfg("samplerate", samplerate), res);
                    res = std::max(rrc_blk.set_cfg("symbolrate", symbolrate), res);
                    res = std::max(rec_blk.set_cfg("omega", samplerate / symbolrate), res);
                    return res;
                }
                else if (key == "advanced")
                {
                    advanced_mode = v;
                    return RES_LISTUPD;
                }
                else if (key.find("rrc_") == 0)
                {
                    replaceAllStr(key, "rrc_", "");
                    return rrc_blk.set_cfg(key, v);
                }
                else if (key.find("agc_") == 0)
                {
                    replaceAllStr(key, "agc_", "");
                    return agc_blk.set_cfg(key, v);
                }
                else if (key.find("rec_") == 0)
                {
                    replaceAllStr(key, "rec_", "");
                    return rec_blk.set_cfg(key, v);
                }
                else if (key.find("pll_") == 0)
                {
                    replaceAllStr(key, "pll_", "");
                    return pll_blk.set_cfg(key, v);
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