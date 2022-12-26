/*!
 * \file
 * \brief Class tools::Sparse_matrix.
 */
#pragma once

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "matrix.h"

namespace codings
{
	namespace ldpc
	{
		class Sparse_matrix : public Matrix
		{
		public:
			using Idx_t = uint32_t;

			Sparse_matrix(const size_t n_rows = 0, const size_t n_cols = 1);

			virtual ~Sparse_matrix() = default;

			inline const std::vector<Idx_t> &get_cols_from_row(const size_t row_index) const;

			inline const std::vector<Idx_t> &get_rows_from_col(const size_t col_index) const;

			inline const std::vector<Idx_t> &operator[](const size_t col_index) const;

			inline const std::vector<std::vector<Idx_t>> &get_row_to_cols() const;

			inline const std::vector<std::vector<Idx_t>> &get_col_to_rows() const;

			/*
			 * return true if there is a connection there
			 */
			bool at(const size_t row_index, const size_t col_index) const;

			/*
			 * Add a connection
			 */
			void add_connection(const size_t row_index, const size_t col_index);

			/*
			 * Remove the connection
			 */
			void rm_connection(const size_t row_index, const size_t col_index);

			/*
			 * Resize the matrix to the new dimensions in function of the part of the matrix considered as the origin
			 * Ex: if o == BOTTOM_RIGHT, and need to extend the matrix then add rows on the top and columns on the left
			 */
			void self_resize(const size_t n_rows, const size_t n_cols, Origin o);

			/*
			 * Resize the matrix by calling self_resize on a copy matrix
			 */
			Sparse_matrix resize(const size_t n_rows, const size_t n_cols, Origin o) const;

			/*
			 * Return the transposed matrix of this matrix
			 */
			Sparse_matrix transpose() const;

			/*
			 * Transpose internally this matrix
			 */
			void self_transpose();

			/*
			 * Return turn the matrix in horizontal or vertical way
			 */
			Sparse_matrix turn(Way w) const;

			/*
			 * Sort the sparse matrix per density of lines
			 * The "order" parameter can be "ASC" for ascending or "DSC" for descending
			 */
			void sort_cols_per_density(Sort order);

			/*
			 * Print the sparsed matrix in its full view with 0s and 1s.
			 * 'transpose' allow the print in its transposed view
			 */
			void print(bool transpose = false, std::ostream &os = std::cout) const;

			/*
			 * \brief create a matrix of the given size filled with identity diagonal
			 * \return the identity matrix
			 */
			static Sparse_matrix identity(const size_t n_rows, const size_t n_cols);

			/*
			 * \brief create a matrix of the given size filled with only zeros
			 * \return the zero matrix
			 */
			static Sparse_matrix zero(const size_t n_rows, const size_t n_cols);

		private:
			std::vector<std::vector<Idx_t>> row_to_cols;
			std::vector<std::vector<Idx_t>> col_to_rows;

			/*
			 * Compute the rows and cols degrees values when the matrix values have been modified
			 * without the use of 'add_connection' and 'rm_connection' interface, so after any modification
			 * call this function if you need degrees information
			 */
			void parse_connections();
		};
	}
}

namespace codings
{
	namespace ldpc
	{
		const std::vector<uint32_t> &Sparse_matrix ::get_cols_from_row(const size_t row_index) const
		{
			return this->row_to_cols[row_index];
		}

		const std::vector<uint32_t> &Sparse_matrix ::get_rows_from_col(const size_t col_index) const
		{
			return this->col_to_rows[col_index];
		}

		const std::vector<uint32_t> &Sparse_matrix ::operator[](const size_t col_index) const
		{
			return this->get_rows_from_col(col_index);
		}

		const std::vector<std::vector<uint32_t>> &Sparse_matrix ::get_row_to_cols() const
		{
			return this->row_to_cols;
		}

		const std::vector<std::vector<uint32_t>> &Sparse_matrix ::get_col_to_rows() const
		{
			return this->col_to_rows;
		}
	}
}
