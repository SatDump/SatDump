#include "core/plugin.h"
#include "logger.h"
#include "common/cpu_features.h"

#include "ldpc_decoder/ldpc_decoder_avx.h"
#include "common/codings/ldpc/ldpc_decoder.h"

class SimdAVX2 : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "simd_avx2";
    }

    static void registerLDPCDecoder(codings::ldpc::GetLDPCDecodersEvent evt)
    {
        evt.decoder_list.insert({"avx2", [](codings::ldpc::Sparse_matrix pcm)
                                 { return new codings::ldpc::LDPCDecoderAVX(pcm); }});
    }

    void init()
    {
        if (!cpu_features::get_cpu_features().CPU_X86_AVX2)
        {
            logger->error("CPU Does not support AVX2. Extension plugin NOT loading!");
            return;
        }

        satdump::eventBus->register_handler<codings::ldpc::GetLDPCDecodersEvent>(registerLDPCDecoder);
    }
};

PLUGIN_LOADER(SimdAVX2)