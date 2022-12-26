#include <algorithm>
#include <numeric>
#include <sstream>
#include <ios>

// #include "Tools/Exception/exception.hpp"
#include "sparse_matrix.h"

using namespace codings;
using namespace codings::ldpc;

Sparse_matrix ::Sparse_matrix(const size_t n_rows, const size_t n_cols)
	: Matrix(n_rows, n_cols),
	  row_to_cols(n_rows),
	  col_to_rows(n_cols)
{
}

bool Sparse_matrix ::at(const size_t row_index, const size_t col_index) const
{
	auto it = std::find(this->row_to_cols[row_index].begin(), this->row_to_cols[row_index].end(), col_index);
	return (it != this->row_to_cols[row_index].end());
}

void Sparse_matrix ::add_connection(const size_t row_index, const size_t col_index)
{
	check_indexes(row_index, col_index);

	if (at(row_index, col_index))
	{
		std::stringstream message;
		message << "('row_index';'col_index') connection already exists ('row_index' = " << row_index
				<< ", 'col_index' = " << col_index << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	this->row_to_cols[row_index].push_back((uint32_t)col_index);
	this->col_to_rows[col_index].push_back((uint32_t)row_index);

	this->rows_max_degree = std::max(get_rows_max_degree(), row_to_cols[row_index].size());
	this->cols_max_degree = std::max(get_cols_max_degree(), col_to_rows[col_index].size());

	this->n_connections++;
}

void Sparse_matrix ::rm_connection(const size_t row_index, const size_t col_index)
{
	check_indexes(row_index, col_index);

	// delete the link in the row_to_cols vector
	bool row_found = false;
	auto itr = std::find(this->row_to_cols[row_index].begin(), this->row_to_cols[row_index].end(), col_index);
	if (itr != this->row_to_cols[row_index].end())
	{
		row_found = true;
		itr = this->row_to_cols[row_index].erase(itr);

		// check if need to reduce the rows max degree
		if (this->row_to_cols[row_index].size() == (this->rows_max_degree - 1))
		{
			bool found = false;
			for (auto i = this->row_to_cols.begin(); i != this->row_to_cols.end(); i++)
				if (i->size() == this->rows_max_degree)
				{
					found = true;
					break;
				}

			if (!found)
				this->rows_max_degree--;
		}
	}

	// delete the link in the col_to_rows vector
	bool col_found = false;
	auto itc = std::find(this->col_to_rows[col_index].begin(), this->col_to_rows[col_index].end(), row_index);
	if (itc != this->col_to_rows[col_index].end())
	{
		col_found = true;
		this->col_to_rows[col_index].erase(itc);

		// check if need to reduce the cols max degree
		if (this->col_to_rows[col_index].size() == (this->cols_max_degree - 1))
		{
			bool found = false;
			for (auto i = this->col_to_rows.begin(); i != this->col_to_rows.end(); i++)
				if (i->size() == this->cols_max_degree)
				{
					found = true;
					break;
				}

			if (!found)
				this->cols_max_degree--;
		}
	}

	if (row_found != col_found)
	{
		std::stringstream message;
		message << "The connection has been found only in one of the two vectors 'row_to_cols' and 'col_to_rows' "
				<< "('row_index' = " << row_index << ", 'col_index' = " << col_index
				<< ", found in row = " << std::boolalpha << row_found
				<< ", found in col = " << col_found << std::noboolalpha << ").";
		throw std::runtime_error(message.str()); // tools::runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
	else if (row_found && col_found)
		this->n_connections--;
	// else the connection has not been found
}

void Sparse_matrix ::parse_connections()
{
	this->n_connections = std::accumulate(row_to_cols.begin(), row_to_cols.end(), (size_t)0,
										  [](size_t init, const std::vector<Idx_t> &a)
										  { return init + a.size(); });

	this->rows_max_degree = std::max_element(row_to_cols.begin(), row_to_cols.end(),
											 [](const std::vector<Idx_t> &a, const std::vector<Idx_t> &b)
											 { return a.size() < b.size(); })
								->size();
	this->cols_max_degree = std::max_element(col_to_rows.begin(), col_to_rows.end(),
											 [](const std::vector<Idx_t> &a, const std::vector<Idx_t> &b)
											 { return a.size() < b.size(); })
								->size();
}

void Sparse_matrix ::self_resize(const size_t n_rows, const size_t n_cols, Origin o)
{
	*this = this->resize(n_rows, n_cols, o);
}

Sparse_matrix Sparse_matrix ::resize(const size_t n_rows, const size_t n_cols, Origin o) const
{
	Sparse_matrix resized(n_rows, n_cols);

	// const auto min_r = std::min(n_rows, get_n_rows());
	const auto min_c = std::min(n_cols, get_n_cols());
	// const auto diff_r = get_n_rows() - min_r;
	const auto diff_c = get_n_cols() - min_c;
	const auto diff_n_rows = (int)n_rows - (int)get_n_rows();
	const auto diff_n_cols = (int)n_cols - (int)get_n_cols();

	switch (o)
	{
	case Origin::TOP_LEFT:
	{
		for (size_t c = 0; c < min_c; c++)
			for (size_t r = 0; r < col_to_rows[c].size(); r++)
			{
				const auto col_index = c;
				const auto row_index = col_to_rows[c][r];
				if (row_index < n_rows)
				{
					resized.row_to_cols[row_index].push_back((uint32_t)col_index);
					resized.col_to_rows[col_index].push_back((uint32_t)row_index);
				}
			}
	}
	break;
	case Origin::TOP_RIGHT:
	{
		for (size_t c = diff_c; c < get_n_cols(); c++)
			for (size_t r = 0; r < col_to_rows[c].size(); r++)
			{
				const auto col_index = diff_n_cols + c;
				const auto row_index = col_to_rows[c][r];
				if (row_index < n_rows)
				{
					resized.row_to_cols[row_index].push_back((uint32_t)col_index);
					resized.col_to_rows[col_index].push_back((uint32_t)row_index);
				}
			}
	}
	break;
	case Origin::BOTTOM_LEFT:
	{
		for (size_t c = 0; c < min_c; c++)
			for (size_t r = 0; r < col_to_rows[c].size(); r++)
			{
				const auto col_index = c;
				const auto row_index = diff_n_rows + (int)col_to_rows[c][r];
				if (row_index >= 0)
				{
					resized.row_to_cols[row_index].push_back((uint32_t)col_index);
					resized.col_to_rows[col_index].push_back((uint32_t)row_index);
				}
			}
	}
	break;
	case Origin::BOTTOM_RIGHT:
	{
		for (size_t c = diff_c; c < get_n_cols(); c++)
			for (size_t r = 0; r < col_to_rows[c].size(); r++)
			{
				const auto col_index = diff_n_cols + c;
				const auto row_index = diff_n_rows + (int)col_to_rows[c][r];
				if (row_index >= 0)
				{
					resized.row_to_cols[row_index].push_back((uint32_t)col_index);
					resized.col_to_rows[col_index].push_back((uint32_t)row_index);
				}
			}
	}
	break;
	}

	resized.parse_connections();

	return resized;
}

Sparse_matrix Sparse_matrix ::transpose() const
{
	Sparse_matrix trans(*this);

	trans.self_transpose();

	return trans;
}

void Sparse_matrix ::self_transpose()
{
	Matrix::self_transpose();

	std::swap(row_to_cols, col_to_rows);
}

Sparse_matrix Sparse_matrix ::turn(Way w) const
{
	Sparse_matrix turned(*this);

	turned.self_turn(w);

	return turned;
}

void Sparse_matrix ::sort_cols_per_density(Sort order)
{
	switch (order)
	{
	case Sort::ASCENDING:
		std::sort(this->col_to_rows.begin(), this->col_to_rows.end(),
				  [](const std::vector<Idx_t> &i1, const std::vector<Idx_t> &i2)
				  { return i1.size() < i2.size(); });
		break;
	case Sort::DESCENDING:
		std::sort(this->col_to_rows.begin(), this->col_to_rows.end(),
				  [](const std::vector<Idx_t> &i1, const std::vector<Idx_t> &i2)
				  { return i1.size() > i2.size(); });
		break;
	}

	for (auto &r : this->row_to_cols)
		r.clear();
	for (size_t i = 0; i < this->col_to_rows.size(); i++)
		for (size_t j = 0; j < this->col_to_rows[i].size(); j++)
			this->row_to_cols[this->col_to_rows[i][j]].push_back((uint32_t)i);
}

void Sparse_matrix ::print(bool transpose, std::ostream &os) const
{
	if (transpose)
	{
		std::vector<unsigned> rows(get_n_rows(), 0);

		for (auto &col : this->col_to_rows)
		{
			// set the ones
			for (auto &row : col)
				rows[row] = 1;

			for (auto &row : rows)
				os << row << " ";

			os << std::endl;

			// reset the ones
			for (auto &row : col)
				rows[row] = 0;
		}
	}
	else
	{
		std::vector<unsigned> columns(get_n_cols(), 0);

		for (auto &row : this->row_to_cols)
		{
			// set the ones
			for (auto &col : row)
				columns[col] = 1;

			for (auto &col : columns)
				os << col << " ";

			os << std::endl;

			// reset the ones
			for (auto &col : row)
				columns[col] = 0;
		}
	}
}

Sparse_matrix Sparse_matrix ::identity(const size_t n_rows, const size_t n_cols)
{
	Sparse_matrix mat(n_rows, n_cols);
	auto shortest_side = std::min(n_rows, n_cols);

	for (size_t i = 0; i < shortest_side; i++)
		mat.add_connection(i, i);

	return mat;
}

Sparse_matrix Sparse_matrix ::zero(const size_t n_rows, const size_t n_cols)
{
	return Sparse_matrix(n_rows, n_cols);
}