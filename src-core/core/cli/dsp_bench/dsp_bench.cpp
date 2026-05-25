#include "dsp_bench.h"
#include "dsp/device/dev.h"
#include "logger.h"

namespace satdump
{
    void DspBenchCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_dsp_bench = app->add_subcommand("dsp_bench", "Perform DSP Benchmark");
        //
    }

    void DspBenchCmdHandler::run(CLI::App *app, CLI::App *subcom, bool is_gui) {
        
    }
} // namespace satdump