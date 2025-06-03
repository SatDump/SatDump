#pragma once

#include "common/dsp/path/splitter.h"
#include "pipeline/modules/demod/module_demod_base.h"
#include "stx_demod_hier.h"

namespace orbcomm
{
    class OrbcommSTXAutoDemodModule : public satdump::pipeline::demod::BaseDemodModule
    {
    protected:
        const double d_center_frequency;

        std::shared_ptr<dsp::SplitterBlock> splitter;

        struct STXLink
        {
            std::string id;
            std::shared_ptr<STXDemod> demod;
            time_t last_ping;
        };
        std::mutex stx_link_lst_mtx;
        std::map<double, STXLink> stx_link_lst;

        void add_stx_link(double frequency)
        {
            stx_link_lst_mtx.lock();
            if (stx_link_lst.count(frequency) > 0)
            {
                stx_link_lst[frequency].last_ping = time(0);
            }
            else
            {
                std::string id = std::to_string(frequency);
                double offset = d_center_frequency - frequency;
                splitter->add_vfo(id, d_samplerate, offset);
                STXLink link;
                link.id = id;
                link.demod = std::make_shared<STXDemod>(splitter->get_vfo_output(id), frm_callback, d_samplerate, true);
                link.last_ping = time(0);
                link.demod->start();
                logger->info("Started!");
                splitter->set_vfo_enabled(id, true);
                logger->info("Enabled!");
                stx_link_lst.insert({frequency, link});
                logger->info("Copied!");

                logger->critical("OFFSET %f", offset);
            }
            stx_link_lst_mtx.unlock();
        }

        void del_stx_link(double frequency)
        {
            stx_link_lst_mtx.lock();
            if (stx_link_lst.count(frequency) > 0)
            {
                splitter->set_vfo_enabled(stx_link_lst[frequency].id, false);
                stx_link_lst[frequency].demod->stop();
                splitter->del_vfo(stx_link_lst[frequency].id);
                stx_link_lst.erase(frequency);
            }
            stx_link_lst_mtx.unlock();
        }

        std::mutex freqs_to_push_mtx;
        std::vector<double> freqs_to_push;

    protected:
        std::function<void(uint8_t *, int)> frm_callback;

    public:
        OrbcommSTXAutoDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~OrbcommSTXAutoDemodModule();
        void init();
        void stop();
        void process();

        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace orbcomm