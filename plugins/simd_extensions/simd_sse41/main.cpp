#include "core/plugin.h"
#include "logger.h"
#include "common/cpu_features.h"

#include "ldpc_decoder/ldpc_decoder_sse.h"
#include "common/codings/ldpc/ldpc_decoder.h"

class SimdSSE41 : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "simd_sse41";
    }

    static void registerLDPCDecoder(codings::ldpc::GetLDPCDecodersEvent evt)
    {
        evt.decoder_list.insert({"sse41", [](codings::ldpc::Sparse_matrix pcm)
                                 { return new codings::ldpc::LDPCDecoderSSE(pcm); }});
    }

    void init()
    {
        if (!cpu_features::get_cpu_features().CPU_X86_SSE41)
        {
            logger->error("CPU Does not support SSE4_1. Extension plugin NOT loading!");
            return;
        }

        satdump::eventBus->register_handler<codings::ldpc::GetLDPCDecodersEvent>(registerLDPCDecoder);
    }
};

PLUGIN_LOADER(SimdSSE41)