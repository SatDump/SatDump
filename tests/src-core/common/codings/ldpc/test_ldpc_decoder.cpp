#include "common/codings/ldpc/ldpc_decoder.h"
#include "common/codings/ldpc/ldpc_decoder_generic.h"
#include "common/codings/ldpc/make_ccsds.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

TEST_CASE("get_best_ldpc_decoder falls back to the generic decoder without SIMD plugins", "[common][codings][ldpc]")
{
    auto pcm = codings::ldpc::ccsds_ar4ja::make_ar4ja_code(codings::ldpc::ccsds_ar4ja::AR4JA_RATE_4_5, codings::ldpc::ccsds_ar4ja::AR4JA_BLOCK_1024);
    std::unique_ptr<codings::ldpc::LDPCDecoder> dec(codings::ldpc::get_best_ldpc_decoder(pcm));
    REQUIRE(dec != nullptr);
    CHECK(dec->simd() == 1);
    CHECK(dynamic_cast<codings::ldpc::LDPCDecoderGeneric *>(dec.get()) != nullptr);
}
