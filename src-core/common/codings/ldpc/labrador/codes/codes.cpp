#include "codes.h"
#include "common/codings/ldpc/labrador/codes/compact_generators.h"
#include "compact_parity_checks.h"
#include "core/exception.h"

namespace satdump
{
    namespace labrador
    {
        /// Code parameters for the TC128 code
        const code_params_t TC128_PARAMS = code_params_t{
            TC128,
            128,
            64,
            0,
            128 / 8,
            128 / 8,
            512,

            128 + 0,
            2 * 512 + 3 * 128 + 3 * 0 - 2 * 64,
            (128 + 0 - 64) / 8,
            128 / 8,

            &TC128_H,
            nullptr,
            TC128_G,
        };

        /// Code parameters for the TC256 code
        const code_params_t TC256_PARAMS = code_params_t{
            TC256,
            256,
            128,
            0,
            256 / 8,
            256 / 8,
            1024,

            256 + 0,
            2 * 1024 + 3 * 256 + 3 * 0 - 2 * 128,
            (256 + 0 - 128) / 8,
            256 / 8,

            &TC256_H,
            nullptr,
            TC256_G,
        };

        /// Code parameters for the TC512 code
        const code_params_t TC512_PARAMS = code_params_t{
            TC512,
            512,
            256,
            0,
            512 / 8,
            512 / 8,
            2048,

            512 + 0,
            2 * 2048 + 3 * 512 + 3 * 0 - 2 * 256,
            (512 + 0 - 256) / 8,
            512 / 8,

            &TC512_H,
            nullptr,
            TC512_G,
        };

        /// Code parameters for the TM1280 code
        const code_params_t TM1280_PARAMS = code_params_t{
            TM1280,
            1280,
            1024,
            128,
            128,
            128 / 4,
            4992,

            1280 + 128,
            2 * 4992 + 3 * 1280 + 3 * 128 - 2 * 1024,
            (1280 + 128 - 1024) / 8,
            (1280 + 128) / 8,

            &TM_R45_H,
            &PHI_J_K_M128,
            TM1280_G,
        };

        /// Code parameters for the TM1536 code
        const code_params_t TM1536_PARAMS = code_params_t{
            TM1536,
            1536,
            1024,
            256,
            256,
            256 / 4,
            5888,

            1536 + 256,
            2 * 5888 + 3 * 1536 + 3 * 256 - 2 * 1024,
            (1536 + 256 - 1024) / 8,
            (1536 + 256) / 8,

            &TM_R23_H,
            &PHI_J_K_M256,
            TM1536_G,
        };

        /// Code parameters for the TM2048 code
        const code_params_t TM2048_PARAMS = code_params_t{
            TM2048,
            2048,
            1024,
            512,
            512,
            512 / 4,
            7680,

            2048 + 512,
            2 * 7680 + 3 * 2048 + 3 * 512 - 2 * 1024,
            (2048 + 512 - 1024) / 8,
            (2048 + 512) / 8,

            &TM_R12_H,
            &PHI_J_K_M512,
            TM2048_G,
        };

        /// Code parameters for the TM5120 code
        const code_params_t TM5120_PARAMS = code_params_t{
            TM5120,
            5120,
            4096,
            512,
            512,
            512 / 4,
            19968,

            5120 + 512,
            2 * 19968 + 3 * 5120 + 3 * 512 - 2 * 4096,
            (5120 + 512 - 4096) / 8,
            (5120 + 512) / 8,

            &TM_R45_H,
            &PHI_J_K_M512,
            TM5120_G,
        };

        /// Code parameters for the TM6144 code
        const code_params_t TM6144_PARAMS = code_params_t{
            TM6144,
            6144,
            4096,
            1024,
            1024,
            1024 / 4,
            23552,

            6144 + 1024,
            2 * 23552 + 3 * 6144 + 3 * 1024 - 2 * 4096,
            (6144 + 1024 - 4096) / 8,
            (6144 + 1024) / 8,

            &TM_R23_H,
            &PHI_J_K_M1024,
            TM6144_G,
        };

        /// Code parameters for the TM8192 code
        const code_params_t TM8192_PARAMS = code_params_t{
            TM8192,
            8192,
            4096,
            2048,
            2048,
            2048 / 4,
            30720,

            8192 + 2048,
            2 * 30720 + 3 * 8192 + 3 * 2048 - 2 * 4096,
            (8192 + 2048 - 4096) / 8,
            (8192 + 2048) / 8,

            &TM_R12_H,
            &PHI_J_K_M2048,
            TM8192_G,
        };

        code_params_t get_code_params(ldpc_code_t c)
        {
            // TC Codes
            if (c == TC128)
                return TC128_PARAMS;
            else if (c == TC256)
                return TC256_PARAMS;
            else if (c == TC512)
                return TC512_PARAMS;

            // TM Codes 1024-bits
            else if (c == TM1280)
                return TM1280_PARAMS;
            else if (c == TM1536)
                return TM1536_PARAMS;
            else if (c == TM2048)
                return TM2048_PARAMS;

            // TM Codes 2048-bits
            else if (c == TM5120)
                return TM5120_PARAMS;
            else if (c == TM6144)
                return TM6144_PARAMS;
            else if (c == TM8192)
                return TM8192_PARAMS;

            // Error!
            else
                throw satdump_exception("Invalid LDPC Code!");
        }
    } // namespace labrador
} // namespace satdump