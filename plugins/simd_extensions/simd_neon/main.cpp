#include "core/plugin.h"
#include "logger.h"
#include "common/cpu_features.h"

#include "ldpc_decoder/ldpc_decoder_neon.h"
#include "common/codings/ldpc/ldpc_decoder.h"

class SimdNEON : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "simd_neon";
    }

    static void registerLDPCDecoder(codings::ldpc::GetLDPCDecodersEvent evt)
    {
        evt.decoder_list.insert({"neon", [](codings::ldpc::Sparse_matrix pcm)
                                 { return new codings::ldpc::LDPCDecoderNEON(pcm); }});
    }

    void init()
    {
        if (!cpu_features::get_cpu_features().CPU_ARM_NEON)
        {
            logger->error("CPU Does not support NEON. Extension plugin NOT loading!");
            return;
        }

        satdump::eventBus->register_handler<codings::ldpc::GetLDPCDecodersEvent>(registerLDPCDecoder);
    }
};

PLUGIN_LOADER(SimdNEON)