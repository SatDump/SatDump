#include <algorithm>
#include <sstream>

// #include "Tools/Exception/exception.hpp"
#include "matrix.h"

using namespace codings;
using namespace codings::ldpc;

Matrix ::Matrix(const size_t n_rows, const size_t n_cols)
	: n_rows(n_rows),
	  n_cols(n_cols),
	  rows_max_degree(0),
	  cols_max_degree(0),
	  n_connections(0)
{
}

void Matrix ::self_transpose()
{
	std::swap(n_rows, n_cols);
	std::swap(rows_max_degree, cols_max_degree);
}

void Matrix ::self_turn(Way w)
{
	if (w == Way::VERTICAL)
	{
		if (get_n_cols() > get_n_rows())
			this->self_transpose();
	}
	else if (w == Way::HORIZONTAL)
	{
		if (get_n_cols() < get_n_rows())
			this->self_transpose();
	}
}

Matrix::Way Matrix ::get_way() const
{
	return (get_n_cols() >= get_n_rows()) ? Way::HORIZONTAL : Way::VERTICAL;
}

float Matrix ::compute_density() const
{
	return ((float)n_connections / (float)(n_rows * n_cols));
}

void Matrix ::check_indexes(const size_t row_index, const size_t col_index) const
{
	if (col_index >= get_n_cols())
	{
		std::stringstream message;
		message << "'col_index' has to be smaller than 'n_cols' ('col_index' = " << col_index
				<< ", 'n_cols' = " << get_n_cols() << ").";
		throw std::runtime_error(message.str()); // invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (row_index >= get_n_rows())
	{
		std::stringstream message;
		message << "'row_index' has to be smaller than 'n_rows' ('row_index' = " << row_index
				<< ", 'n_rows' = " << get_n_rows() << ").";
		throw std::runtime_error(message.str()); // invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}
}

void Matrix ::self_resize(const size_t _n_rows, const size_t _n_cols)
{
	n_rows = _n_rows;
	n_cols = _n_cols;
}

bool Matrix ::is_of_way(Way w) const noexcept
{
	return get_way() == w;
}

void Matrix ::is_of_way_throw(Way w) const
{
	if (!is_of_way(w))
	{
		std::stringstream message;
		message << "This matrix way ('" << way_to_str(get_way()) << "') is not same as the given checked one ('"
				<< way_to_str(w) << "').";
		throw std::runtime_error(message.str()); // tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}
}

std::string Matrix ::way_to_str(Way w)
{
	std::string str;

	switch (w)
	{
	case Way::HORIZONTAL:
		str = "HORIZONTAL";
		break;
	case Way::VERTICAL:
		str = "VERTICAL";
		break;
	}

	if (str.empty()) // this 'if' is a test outside the switch case (instead of default) to keep the compiler check that all
					 // cases of 'Way' are well represented.
	{
		std::stringstream message;
		message << "The way 'w' does not represent a matrix way ('w' = " << (short)w << ").";
		throw std::runtime_error(message.str()); // tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	return str;
}

std::ostream &operator<<(std::ostream &os, const Matrix &sm)
{
	sm.print(0, os);
	return os;
}