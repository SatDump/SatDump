#include "dsp_bench.h"
#include "dsp/benchmark/bench.h"
#include "dsp/benchmark/render/plot_results.h"
#include "dsp/device/dev.h"
#include "image/io.h"
#include "logger.h"

namespace satdump
{
    void DspBenchCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_dsp_bench = app->add_subcommand("dsp_bench", "Perform DSP Benchmark");

        auto cats = ndsp::getBenchCategories();

        for (auto &cat : cats)
            sub_dsp_bench->add_flag_callback("--" + cat, [this, cat]() { cats_to_use.push_back(cat); });
    }

    void DspBenchCmdHandler::run(CLI::App *app, CLI::App *subcom, bool is_gui)
    {
        auto results = ndsp::runBenchmarks(cats_to_use);

        auto img = ndsp::renderResults(results);

        image::save_img(img, "dsp_benchmark.png");
    }
} // namespace satdump