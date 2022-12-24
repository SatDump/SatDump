#include "ldpc_decoder.h"
#include "core/plugin.h"

#include "common/cpu_features.h"

#include "ldpc_decoder_generic.h"

namespace codings
{
    namespace ldpc
    {
        LDPCDecoder::LDPCDecoder(Sparse_matrix)
        {
        }

        LDPCDecoder::~LDPCDecoder()
        {
        }

        LDPCDecoder *get_best_ldpc_decoder(Sparse_matrix pcm)
        {
            std::map<std::string, std::function<LDPCDecoder *(Sparse_matrix)>> decoder_list;

            satdump::eventBus->fire_event<GetLDPCDecodersEvent>({decoder_list});
            decoder_list.insert({"generic", [](Sparse_matrix pcm)
                                 { return new LDPCDecoderGeneric(pcm); }});

            cpu_features::cpu_features_t cpu_caps = cpu_features::get_cpu_features();

            if (cpu_caps.CPU_X86_AVX2 && decoder_list.count("avx2") > 0)
                return decoder_list["avx2"](pcm);
            else if (cpu_caps.CPU_X86_SSE41)
                return decoder_list["sse41"](pcm);
            else if (cpu_caps.CPU_ARM_NEON)
                return decoder_list["neon"](pcm);
            else
                return decoder_list["generic"](pcm);
        }
    }
}