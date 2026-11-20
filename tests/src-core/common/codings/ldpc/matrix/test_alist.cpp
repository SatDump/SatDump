#include "common/codings/ldpc/make_ccsds.h"
#include "common/codings/ldpc/matrix/alist.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <sstream>
#include <stdexcept>

namespace
{
    using codings::ldpc::AList;
    using codings::ldpc::Sparse_matrix;

    bool sameMatrix(const Sparse_matrix &a, const Sparse_matrix &b)
    {
        if (a.get_n_rows() != b.get_n_rows() || a.get_n_cols() != b.get_n_cols() || a.get_n_connections() != b.get_n_connections())
            return false;
        for (size_t r = 0; r < a.get_n_rows(); r++)
            for (auto c : a.get_cols_from_row(r))
                if (!b.at(r, c))
                    return false;
        return true;
    }

    Sparse_matrix irregular()
    {
        // Rows of different degree so padding matters
        Sparse_matrix m(3, 5);
        m.add_connection(0, 0);
        m.add_connection(0, 4);
        m.add_connection(1, 1);
        m.add_connection(1, 2);
        m.add_connection(1, 3);
        m.add_connection(2, 0);
        m.add_connection(2, 2);
        return m;
    }
} // namespace

TEST_CASE("AList write/read round-trips with and without zero padding", "[common][codings][ldpc]")
{
    bool padding = GENERATE(false, true);
    INFO("zero padding " << padding);
    Sparse_matrix m = irregular();
    std::stringstream ss;
    AList::write(m, ss, padding);
    Sparse_matrix back = AList::read(ss);
    CHECK(sameMatrix(m, back));
}

TEST_CASE("AList write produces the expected text", "[common][codings][ldpc]")
{
    std::stringstream ss;
    AList::write(irregular(), ss, true);
    CHECK(ss.str() == "3 5\n\n3 2\n\n2 3 2 \n\n2 1 2 1 1 \n\n1 5 0 \n2 3 4 \n1 3 0 \n\n1 3 \n2 0 \n2 3 \n2 0 \n1 0 \n");
}

TEST_CASE("AList round-trips a real CCSDS parity check matrix", "[common][codings][ldpc]")
{
    Sparse_matrix h = codings::ldpc::ccsds_ar4ja::make_ar4ja_code(codings::ldpc::ccsds_ar4ja::AR4JA_RATE_4_5, codings::ldpc::ccsds_ar4ja::AR4JA_BLOCK_1024);
    std::stringstream ss;
    AList::write(h, ss, true);
    CHECK(sameMatrix(h, AList::read(ss)));
}

TEST_CASE("AList read_matrix_size returns the first two header values", "[common][codings][ldpc]")
{
    std::stringstream ss;
    AList::write(irregular(), ss);
    int H = 0, N = 0;
    AList::read_matrix_size(ss, H, N);
    // First value goes to N, second to H
    CHECK(N == 3);
    CHECK(H == 5);
}

TEST_CASE("AList info bits positions round-trip and validation", "[common][codings][ldpc]")
{
    std::vector<unsigned> pos = {0, 3, 4, 7};
    std::stringstream ss;
    ss << "garbage line\n";
    AList::write_info_bits_pos(pos, ss);

    std::stringstream a(ss.str());
    CHECK(AList::read_info_bits_pos(a) == pos);
    std::stringstream b(ss.str());
    CHECK(AList::read_info_bits_pos(b, 4, 8) == pos);
    std::stringstream c(ss.str());
    CHECK_THROWS_AS(AList::read_info_bits_pos(c, 3, 8), std::runtime_error); // wrong K
    std::stringstream d(ss.str());
    CHECK_THROWS_AS(AList::read_info_bits_pos(d, 4, 7), std::runtime_error); // position >= N
}

TEST_CASE("AList read rejects malformed input", "[common][codings][ldpc]")
{
    std::stringstream empty("");
    CHECK_THROWS_AS(AList::read(empty), std::runtime_error);
    std::stringstream junk("this is not an alist\n");
    CHECK_THROWS_AS(AList::read(junk), std::runtime_error);
}
