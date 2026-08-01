#include "core/plugin.h"
#include "logger.h"

#include "elektro_arktika/instruments/msugs/module_msugs.h"
#include "elektro_arktika/instruments/ggak/module_ggak.h"
#include "elektro_arktika/instruments/ggak/ggak_product.h"
#include "elektro_arktika/instruments/ggak/ggak_product_handler.h"
#include "elektro_arktika/lrit/module_elektro_lrit_data_decoder.h"

class ElektroArktikaSupport : public satdump::Plugin
{
public:
    std::string getID() { return "elektro_arktika_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::products::RegisterProductsEvent>(registerProductsHandler);
        satdump::eventBus->register_handler<satdump::handlers::RequestProductHandlerEvent>(registerProductHandlerHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, elektro_arktika::msugs::MSUGSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, elektro_arktika::ggak::GGAKDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, elektro::lrit::ELEKTROLRITDataDecoderModule);
    }

    static void registerProductsHandler(const satdump::products::RegisterProductsEvent &evt)
    {
        evt.product_loaders.emplace("ggak", satdump::products::RegisteredProduct{
            [](std::string file) -> std::shared_ptr<satdump::products::Product> {
                auto product = std::make_shared<elektro_arktika::ggak::GGAKProduct>();
                product->load(file);
                return product;
            }});
    }

    static void registerProductHandlerHandler(const satdump::handlers::RequestProductHandlerEvent &evt)
    {
        if (evt.product->type == "ggak")
            evt.handler = std::make_shared<elektro_arktika::ggak::GGAKProductHandler>(evt.product, evt.dataset_mode);
    }
};

PLUGIN_LOADER(ElektroArktikaSupport)