#include "common/codings/ldpc/matrix/sparse_matrix.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <stdexcept>

namespace
{
    using codings::ldpc::Matrix;
    using codings::ldpc::Sparse_matrix;

    //  1 0 1 0
    //  0 1 1 0
    //  1 1 1 1
    Sparse_matrix sample()
    {
        Sparse_matrix m(3, 4);
        m.add_connection(0, 0);
        m.add_connection(0, 2);
        m.add_connection(1, 1);
        m.add_connection(1, 2);
        m.add_connection(2, 0);
        m.add_connection(2, 1);
        m.add_connection(2, 2);
        m.add_connection(2, 3);
        return m;
    }

    std::string dense(const Sparse_matrix &m)
    {
        std::string s;
        for (size_t r = 0; r < m.get_n_rows(); r++)
        {
            for (size_t c = 0; c < m.get_n_cols(); c++)
                s += m.at(r, c) ? '1' : '0';
            s += '\n';
        }
        return s;
    }
} // namespace

TEST_CASE("Sparse_matrix add_connection and degrees", "[common][codings][ldpc]")
{
    Sparse_matrix m = sample();
    CHECK(m.get_n_rows() == 3);
    CHECK(m.get_n_cols() == 4);
    CHECK(m.size() == 12);
    CHECK(m.get_n_connections() == 8);
    CHECK(m.get_rows_max_degree() == 4);
    CHECK(m.get_cols_max_degree() == 3);
    CHECK(m.compute_density() == Catch::Approx(8.0 / 12.0));
    CHECK(dense(m) == "1010\n0110\n1111\n");
    CHECK(m.get_cols_from_row(1) == std::vector<uint32_t>({1, 2}));
    CHECK(m.get_rows_from_col(2) == std::vector<uint32_t>({0, 1, 2}));
    CHECK(m[3] == std::vector<uint32_t>({2}));
}

TEST_CASE("Sparse_matrix rejects duplicate and out-of-range connections", "[common][codings][ldpc]")
{
    Sparse_matrix m = sample();
    CHECK_THROWS_AS(m.add_connection(0, 0), std::runtime_error);
    CHECK_THROWS_AS(m.add_connection(3, 0), std::runtime_error);
    CHECK_THROWS_AS(m.add_connection(0, 4), std::runtime_error);
    CHECK_THROWS_AS(m.rm_connection(0, 9), std::runtime_error);
    CHECK(m.get_n_connections() == 8);
}

TEST_CASE("Sparse_matrix rm_connection updates connections and max degrees", "[common][codings][ldpc]")
{
    Sparse_matrix m = sample();
    m.rm_connection(2, 3);
    CHECK_FALSE(m.at(2, 3));
    CHECK(m.get_n_connections() == 7);
    CHECK(m.get_rows_max_degree() == 3);
    CHECK(m.get_cols_max_degree() == 3);

    m.rm_connection(2, 2);
    CHECK(m.get_cols_max_degree() == 2);

    // Removing a non-existing connection is a no-op
    m.rm_connection(0, 1);
    CHECK(m.get_n_connections() == 6);
    CHECK(dense(m) == "1010\n0110\n1100\n");
}

TEST_CASE("Sparse_matrix transpose and turn", "[common][codings][ldpc]")
{
    Sparse_matrix m = sample();
    CHECK(m.get_way() == Matrix::Way::HORIZONTAL);
    CHECK(m.is_of_way(Matrix::Way::HORIZONTAL));
    CHECK_THROWS_AS(m.is_of_way_throw(Matrix::Way::VERTICAL), std::runtime_error);

    Sparse_matrix t = m.transpose();
    CHECK(t.get_n_rows() == 4);
    CHECK(t.get_n_cols() == 3);
    CHECK(t.get_rows_max_degree() == m.get_cols_max_degree());
    CHECK(t.get_cols_max_degree() == m.get_rows_max_degree());
    for (size_t r = 0; r < 3; r++)
        for (size_t c = 0; c < 4; c++)
            REQUIRE(t.at(c, r) == m.at(r, c));

    CHECK(m.turn(Matrix::Way::VERTICAL).get_n_rows() == 4);
    CHECK(m.turn(Matrix::Way::HORIZONTAL).get_n_rows() == 3); // already horizontal
    CHECK(Matrix::way_to_str(Matrix::Way::VERTICAL) == "VERTICAL");
}

TEST_CASE("Sparse_matrix resize keeps the selected corner", "[common][codings][ldpc]")
{
    Sparse_matrix m = sample();

    Sparse_matrix tl = m.resize(2, 3, Matrix::Origin::TOP_LEFT);
    CHECK(dense(tl) == "101\n011\n");
    Sparse_matrix br = m.resize(2, 3, Matrix::Origin::BOTTOM_RIGHT);
    CHECK(dense(br) == "110\n111\n");
    Sparse_matrix tr = m.resize(2, 3, Matrix::Origin::TOP_RIGHT);
    CHECK(dense(tr) == "010\n110\n");
    Sparse_matrix bl = m.resize(2, 3, Matrix::Origin::BOTTOM_LEFT);
    CHECK(dense(bl) == "011\n111\n");

    // Growing pads with zeros on the opposite side
    Sparse_matrix grown = m.resize(4, 5, Matrix::Origin::BOTTOM_RIGHT);
    CHECK(dense(grown) == "00000\n01010\n00110\n01111\n");
    CHECK(grown.get_n_connections() == 8);

    m.self_resize(3, 2, Matrix::Origin::TOP_LEFT);
    CHECK(dense(m) == "10\n01\n11\n");
    CHECK(m.get_n_connections() == 4);
}

TEST_CASE("Sparse_matrix identity, zero, sort and print", "[common][codings][ldpc]")
{
    Sparse_matrix id = Sparse_matrix::identity(3, 5);
    CHECK(dense(id) == "10000\n01000\n00100\n");
    CHECK(id.get_n_connections() == 3);
    Sparse_matrix z = Sparse_matrix::zero(2, 2);
    CHECK(dense(z) == "00\n00\n");
    CHECK(z.get_n_connections() == 0);

    Sparse_matrix m = sample();
    m.sort_cols_per_density(Matrix::Sort::ASCENDING);
    CHECK(m.get_rows_from_col(0).size() == 1);
    CHECK(m.get_rows_from_col(3).size() == 3);
    // row_to_cols is rebuilt consistently with the sorted columns
    for (size_t c = 0; c < 4; c++)
        for (auto r : m.get_rows_from_col(c))
            REQUIRE(m.at(r, c));

    m.sort_cols_per_density(Matrix::Sort::DESCENDING);
    CHECK(m.get_rows_from_col(0).size() == 3);

    std::ostringstream os;
    sample().print(false, os);
    CHECK(os.str() == "1 0 1 0 \n0 1 1 0 \n1 1 1 1 \n");
    std::ostringstream ot;
    sample().print(true, ot);
    CHECK(ot.str() == "1 0 1 \n0 1 1 \n1 1 1 \n0 0 1 \n");
}
