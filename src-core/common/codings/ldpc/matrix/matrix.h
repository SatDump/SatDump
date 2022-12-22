/*!
 * \file
 * \brief Class tools::Matrix.
 */
#pragma once

#include <iostream>
#include <string>

namespace codings
{
	namespace ldpc
	{
		class Matrix
		{
		public:
			enum class Way
			{
				HORIZONTAL, // meaning longest side depicts columns,
				VERTICAL	// meaning longest side depicts rows
			};
			static std::string way_to_str(Way w);

			enum class Sort
			{
				ASCENDING, // meaning longest side depicts columns,
				DESCENDING // meaning longest side depicts rows
			};

			enum class Origin
			{
				TOP_LEFT,
				TOP_RIGHT,
				BOTTOM_LEFT,
				BOTTOM_RIGHT,
			};

			Matrix(const size_t n_rows = 0, const size_t n_cols = 1);

			virtual ~Matrix() = default;

			inline size_t get_n_rows() const;

			inline size_t get_n_cols() const;

			inline size_t size() const;

			inline size_t get_rows_max_degree() const;

			inline size_t get_cols_max_degree() const;

			inline size_t get_n_connections() const;

			/*
			 * return true if there is a connection there
			 */
			virtual bool at(const size_t row_index, const size_t col_index) const = 0;

			/*
			 * Check id indexes are in the matrix bounds
			 */
			void check_indexes(const size_t row_index, const size_t col_index) const;

			/*
			 * Add a connection
			 */
			virtual void add_connection(const size_t row_index, const size_t col_index) = 0;

			/*
			 * Remove the connection
			 */
			virtual void rm_connection(const size_t row_index, const size_t col_index) = 0;

			/*
			 * Resize the matrix to the new dimensions in function of the part of the matrix considered as the origin
			 * Ex: if o == BOTTOM_RIGHT, and need to extend the matrix then add rows on the top and columns on the left
			 */
			virtual void self_resize(const size_t n_rows, const size_t n_cols, Origin o) = 0;

			/*
			 * Transpose internally this matrix
			 */
			virtual void self_transpose();

			/*
			 * \return the way the matrix is : horizontal or vertical
			 * if scaren then it is horizontal
			 */
			Way get_way() const;

			/*
			 * \brief turn the matrix in horizontal or vertical way, when square do nothing
			 */
			void self_turn(Way w);

			/*
			 * Return the density of ones in this matrix
			 */
			float compute_density() const;

			/*
			 * Sort the matrix per density of lines in ascending or descending order
			 */
			virtual void sort_cols_per_density(Sort s) = 0;

			/*
			 * Print the sparsed matrix in its full view with 0s and 1s.
			 * 'transpose' allow the print in its transposed view
			 */
			virtual void print(bool transpose = false, std::ostream &os = std::cout) const = 0;

			bool is_of_way(Way w) const noexcept;
			void is_of_way_throw(Way w) const;

		private:
			size_t n_rows;
			size_t n_cols;

		protected:
			/*
			 * Change n_rows and n_cols with the new values
			 */
			void self_resize(const size_t n_rows, const size_t n_cols);

			size_t rows_max_degree;
			size_t cols_max_degree;
			size_t n_connections;
		};
	}
}

std::ostream &operator<<(std::ostream &os, const codings::ldpc::Matrix &sm);

namespace codings
{
	namespace ldpc
	{
		size_t Matrix ::get_n_rows() const
		{
			return this->n_rows;
		}

		size_t Matrix ::get_n_cols() const
		{
			return this->n_cols;
		}

		size_t Matrix ::size() const
		{
			return this->n_rows * this->n_cols;
		}

		size_t Matrix ::get_rows_max_degree() const
		{
			return this->rows_max_degree;
		}

		size_t Matrix ::get_cols_max_degree() const
		{
			return this->cols_max_degree;
		}

		size_t Matrix ::get_n_connections() const
		{
			return this->n_connections;
		}
	}
}
