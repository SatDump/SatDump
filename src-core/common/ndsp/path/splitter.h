#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

#include <map>
#include <string>
#include <mutex>

namespace ndsp
{
    class Splitter : public ndsp::Block
    {
    private:
        void work();

        std::mutex state_mutex;

        bool enable_main = true;

        struct OutputCFG
        {
            std::shared_ptr<NaFiFo> output_stream;
            bool enabled;
        };

        std::map<std::string, OutputCFG> c_outputs;

    public:
        // Copy outputs
        void add_output(std::string id);
        void del_output(std::string id);
        void reset_output(std::string id);
        std::shared_ptr<NaFiFo> sget_output(std::string id);
        void set_enabled(std::string id, bool enable);

        // Main output
        void set_main_enabled(bool enable);

    public:
        Splitter();

        void set_params(nlohmann::json p = {});
        void start();
        void stop();
    };
}