#include "core/plugin.h"
#include "dsp/block_helpers.h"
#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "dsp/flowgraph/node_int.h"
#include "logger.h"
#include "m10/m10_decoder.h"
#include "m10/m10_parser.h"

class RadioSondeSupport : public satdump::Plugin
{
public:
    std::string getID() { return "radiosonde_support"; }

    void init()
    {
        // satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);

        satdump::eventBus->register_handler<satdump::ndsp::flowgraph::RegisterNodesEvent>(registerNodes);

        satdump::eventBus->register_handler<satdump::ndsp::RequestBlockEvent>(provideBlocks);
    }

    // static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, aim::instruments::AIMInstrumentsDecoderModule); }

    static void registerNodes(const satdump::ndsp::flowgraph::RegisterNodesEvent &evt)
    {
        evt.r.insert({"m10_decoder_hh",
                      {"RadioSonde/M10 Decoder",
                       [](const satdump::ndsp::flowgraph::Flowgraph *f) { return std::make_shared<satdump::ndsp::flowgraph::NodeInternal>(f, std::make_shared<satdump::ndsp::M10DecoderBlock>()); }}});
        evt.r.insert({"m10_parser_h",
                      {"RadioSonde/M10 Parser",
                       [](const satdump::ndsp::flowgraph::Flowgraph *f) { return std::make_shared<satdump::ndsp::flowgraph::NodeInternal>(f, std::make_shared<satdump::ndsp::M10ParserBlock>()); }}});
    }

    static void provideBlocks(const satdump::ndsp::RequestBlockEvent &evt)
    {
        if (evt.id == "m10_decoder_hh")
            evt.blk.push_back(std::make_shared<satdump::ndsp::M10DecoderBlock>());
        if (evt.id == "m10_parser_h")
            evt.blk.push_back(std::make_shared<satdump::ndsp::M10ParserBlock>());
    }
};

PLUGIN_LOADER(RadioSondeSupport)