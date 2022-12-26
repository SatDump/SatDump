#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <string>
#include <iomanip>

// #include "exception.hpp"
#include "general_utils.h"
// #include "Tools/Display/rang_format/rang_format.h"

#include "alist.h"

using namespace codings::ldpc;

Sparse_matrix AList ::read(std::istream &stream)
{
	// save the init pos of the stream
	auto init_pos = stream.tellg();

	try
	{
		return AList::read_format1(stream);
	}
	catch (std::exception const &e1)
	{
		// auto save = exception::no_backtrace;
		// exception::no_backtrace = true;

		std::stringstream message;
		message << "The given stream does not refer to a AList format file: ";
		message << std::endl
				<< e1.what();
		// exception::no_backtrace = save;

		try
		{
			stream.clear();
			stream.seekg(init_pos);
			return AList::read_format2(stream);
		}
		catch (std::exception const &e2)
		{
			// auto save = exception::no_backtrace;
			// exception::no_backtrace = true;
			message << std::endl
					<< e2.what();
			// exception::no_backtrace = save;

			try
			{
				stream.clear();
				stream.seekg(init_pos);
				return AList::read_format3(stream);
			}
			catch (std::exception const &e3)
			{
				// auto save = exception::no_backtrace;
				// exception::no_backtrace = true;
				message << std::endl
						<< e3.what();
				// exception::no_backtrace = save;

				throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
			}
		}
	}
}

void AList ::write(const Sparse_matrix &matrix, std::ostream &stream, bool zero_padding)
{
	stream << matrix.get_n_rows() << " " << matrix.get_n_cols() << std::endl
		   << std::endl;
	stream << matrix.get_rows_max_degree() << " " << matrix.get_cols_max_degree() << std::endl
		   << std::endl;

	for (const auto &r : matrix.get_row_to_cols())
		stream << r.size() << " ";
	stream << std::endl
		   << std::endl;

	for (const auto &c : matrix.get_col_to_rows())
		stream << c.size() << " ";
	stream << std::endl
		   << std::endl;

	for (const auto &r : matrix.get_row_to_cols())
	{
		unsigned i;
		for (i = 0; i < r.size(); i++)
			stream << (r[i] + 1) << " ";
		if (zero_padding)
			for (; i < matrix.get_rows_max_degree(); i++)
				stream << 0 << " ";
		else if (r.size() == 0)
			stream << 0 << " ";

		stream << std::endl;
	}
	stream << std::endl;

	for (const auto &c : matrix.get_col_to_rows())
	{
		unsigned i;
		for (i = 0; i < c.size(); i++)
			stream << (c[i] + 1) << " ";
		if (zero_padding)
			for (; i < matrix.get_cols_max_degree(); i++)
				stream << 0 << " ";
		else if (c.size() == 0)
			stream << 0 << " ";
		stream << std::endl;
	}
}

void AList ::read_matrix_size(std::istream &stream, int &H, int &N)
{
	std::string line;

	getline(stream, line);
	auto values = split(line);
	if (values.size() < 2)
	{
		std::stringstream message;
		message << "'values.size()' has to be greater than 1 ('values.size()' = " << values.size() << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	N = std::stoi(values[0]);
	H = std::stoi(values[1]);
}

std::vector<unsigned> AList ::read_info_bits_pos(std::istream &stream)
{
	std::string line;

	// look for the position in the file where the info bits begin
	while (std::getline(stream, line))
		if (line == "# Positions of the information bits in the codewords:" || stream.eof() || stream.fail() || stream.bad())
			break;

	getline(stream, line);
	auto values = split(line);
	if (values.size() != 1)
	{
		std::stringstream message;
		message << "'values.size()' has to be equal to 1 ('values.size()' = " << values.size() << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	const unsigned size = std::stoi(values[0]);

	getline(stream, line);
	values = split(line);
	if (values.size() != size)
	{
		std::stringstream message;
		message << "'values.size()' has to be equal to 'size' ('values.size()' = " << values.size()
				<< ", 'size' = " << size << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	std::vector<unsigned> info_bits_pos;
	for (auto v : values)
	{
		const unsigned pos = std::stoi(v);

		if (std::find(info_bits_pos.begin(), info_bits_pos.end(), pos) != info_bits_pos.end())
		{
			std::stringstream message;
			message << "'pos' already exists in the 'info_bits_pos' vector ('pos' = " << pos << ").";
			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		}

		info_bits_pos.push_back(pos);
	}

	return info_bits_pos;
}

std::vector<unsigned> AList ::read_info_bits_pos(std::istream &stream, const int K, const int N)
{
	auto info_bits_pos = read_info_bits_pos(stream);

	if (info_bits_pos.size() != (unsigned)K)
	{
		std::stringstream message;
		message << "'info_bits_pos.size()' has to be equal to 'K' ('info_bits_pos.size()' = " << info_bits_pos.size()
				<< ", 'K' = " << K << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	for (auto pos : info_bits_pos)
		if (pos >= (unsigned)N)
		{
			std::stringstream message;
			message << "'pos' has to be smaller than 'N' ('pos' = " << pos << ", 'N' = " << N << ").";
			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		}

	return info_bits_pos;
}

void AList ::write_info_bits_pos(const std::vector<unsigned> &info_bits_pos, std::ostream &stream)
{
	stream << "# Positions of the information bits in the codewords:" << std::endl;
	stream << info_bits_pos.size() << std::endl;
	for (auto pos : info_bits_pos)
		stream << pos << " ";
	stream << std::endl;
}

// perfect AList format
Sparse_matrix AList ::read_format1(std::istream &stream)
{
	unsigned n_rows = 0, n_cols = 0, rows_max_degree = 0, cols_max_degree = 0;

	stream >> n_rows;
	stream >> n_cols;
	stream >> rows_max_degree;
	stream >> cols_max_degree;

	if (n_rows > 0 && n_cols > 0 && rows_max_degree > 0 && cols_max_degree > 0)
	{
		Sparse_matrix matrix(n_rows, n_cols);

		std::vector<unsigned> rows_degree(n_rows);
		for (unsigned i = 0; i < n_rows; i++)
		{
			unsigned n_connections = 0;
			stream >> n_connections;

			if (n_connections == 0 || n_connections > rows_max_degree)
			{
				std::stringstream message;
				message << "'n_connections' has to be greater than 0 and smaller or equal to 'rows_max_degree' "
						<< " ('n_connections' = " << n_connections
						<< ", 'rows_max_degree' = " << rows_max_degree << ").";
				throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
			}

			rows_degree[i] = n_connections;
		}

		// std::vector<unsigned> cols_degree(n_cols);
		for (unsigned i = 0; i < n_cols; i++)
		{
			unsigned n_connections = 0;
			stream >> n_connections;

			if (n_connections == 0 || n_connections > cols_max_degree)
			{
				std::stringstream message;
				message << "'n_connections' has to be greater than 0 and smaller or equal to 'cols_max_degree' "
						<< "('n_connections' = " << n_connections
						<< ", 'cols_max_degree' = " << cols_max_degree << ").";
				throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
			}

			// cols_degree[i] = n_connections;
		}

		for (unsigned i = 0; i < n_rows; i++)
		{
			for (unsigned j = 0; j < rows_max_degree; j++)
			{
				unsigned col_index = 0;

				stream >> col_index;

				if ((col_index > 0 && j < rows_degree[i]) ||
					(col_index == 0 && j >= rows_degree[i]))
				{
					if (col_index)
						matrix.add_connection(i, col_index - 1);
				}
				else
				{
					std::stringstream message;
					message << "'col_index' is wrong ('col_index' = " << col_index << ").";
					throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
				}
			}
		}

		// TODO: this verif. is time consuming
		//		for (unsigned i = 0; i < n_cols; i++)
		//		{
		//			for (unsigned j = 0; j < cols_max_degree; j++)
		//			{
		//				unsigned row_index = 0;
		//				stream >> row_index;
		//
		//				if ((row_index >  0 && j <  cols_degree[i]) ||
		//					(row_index <= 0 && j >= cols_degree[i]))
		//				{
		//					if (row_index)
		//					{
		//						try
		//						{
		//							matrix.add_connection(row_index -1, i);
		//							throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ "The input AList is not consistent.");
		//						}
		//						catch (std::exception const&)
		//						{
		//							// everything is normal if the code passes through the catch
		//						}
		//					}
		//				}
		//				else
		//				{
		//					std::stringstream message;
		//					message << "'row_index' is wrong ('row_index' = " << row_index << ").";
		//					throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		//				}
		//			}
		//		}

		return matrix;
	}
	else
	{
		std::stringstream message;
		message << "'n_rows', 'n_cols', 'rows_max_degree' and 'cols_max_degree' have to be greater than 0 "
				<< "('n_rows' = " << n_rows << ", 'n_cols' = " << n_cols
				<< ", 'rows_max_degree' = " << rows_max_degree << ", 'cols_max_degree' = " << cols_max_degree << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}
}

// format with comments or/and no padding with 0
Sparse_matrix AList ::read_format2(std::istream &stream)
{
	std::string line;
	bool warning_message_displayed = false;

	getline(stream, line);
	auto values = split(line);
	if (values.size() < 2)
	{
		std::stringstream message;
		message << "'values.size()' has to be greater than 1 ('values.size()' = " << values.size() << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	unsigned n_rows = 0, n_cols = 0, rows_max_degree = 0, cols_max_degree = 0;

	n_rows = std::stoi(values[0]);
	n_cols = std::stoi(values[1]);

	if (n_rows == 0 || n_cols == 0)
	{
		std::stringstream message;
		message << "'n_rows' and 'n_cols' have to be greater than 0 ('n_rows' = " << n_rows
				<< ", 'n_cols' = " << n_cols << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	Sparse_matrix matrix(n_rows, n_cols);

	getline(stream, line);
	values = split(line);
	if (values.size() < 2)
	{
		std::stringstream message;
		message << "'values.size()' has to be greater than 1 ('values.size()' = " << values.size() << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	rows_max_degree = std::stoi(values[0]);
	cols_max_degree = std::stoi(values[1]);

	if (rows_max_degree == 0 || cols_max_degree == 0)
	{
		std::stringstream message;
		message << "'rows_max_degree' and 'cols_max_degree' have to be greater than 0 ('rows_max_degree' = "
				<< rows_max_degree << ", 'cols_max_degree' = " << cols_max_degree << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	getline(stream, line);
	values = split(line);
	if (values.size() < n_rows)
	{
		std::stringstream message;
		message << "'values.size()' has to be greater or equal to 'n_rows' ('values.size()' = " << values.size()
				<< ", 'n_rows' = " << n_rows << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	std::vector<unsigned> rows_degree(n_rows);
	for (unsigned i = 0; i < n_rows; i++)
	{
		unsigned n_connections = std::stoi(values[i]);
		if (!warning_message_displayed && n_connections == 0)
		{
			std::clog /*<< rang::tag::warning*/ << "Found in Alist file connections of degree 0!" << std::endl;
			warning_message_displayed = true;
		}

		if (n_connections <= rows_max_degree)
			rows_degree[i] = n_connections;
		else
		{
			std::stringstream message;
			message << "'n_connections' has to be smaller than 'rows_max_degree' "
					<< "('n_connections' = " << n_connections << ", 'rows_max_degree' = " << rows_max_degree << ").";
			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		}
	}

	getline(stream, line);
	values = split(line);
	if (values.size() < n_cols)
	{
		std::stringstream message;
		message << "'values.size()' has to be greater or equal to 'n_cols' ('values.size()' = " << values.size()
				<< ", 'n_cols' = " << n_cols << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	// std::vector<unsigned> cols_degree(n_rows);
	for (unsigned i = 0; i < n_cols; i++)
	{
		unsigned n_connections = std::stoi(values[i]);
		if (!(n_connections <= cols_max_degree))
		{
			std::stringstream message;
			message << "'n_connections' has to be smaller than 'cols_max_degree' "
					<< "('n_connections' = " << n_connections << ", 'cols_max_degree' = " << cols_max_degree << ").";
			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		}
		// else
		// cols_degree[i] = n_connections;
	}

	for (unsigned i = 0; i < n_rows; i++)
	{
		getline(stream, line);
		values = split(line);

		if (values.size() < rows_degree[i])
		{
			std::stringstream message;
			message << "'values.size()' has to be greater or equal to 'rows_degree[i]' ('values.size()' = "
					<< values.size() << ", 'i' = " << i << ", 'rows_degree[i]' = " << rows_degree[i] << ").";
			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		}

		for (unsigned j = 0; j < rows_max_degree; j++)
		{
			auto col_index = (j < values.size()) ? std::stoi(values[j]) : 0;
			if ((col_index > 0 && j < rows_degree[i]) ||
				(col_index <= 0 && j >= rows_degree[i]))
			{
				if (col_index)
					matrix.add_connection(i, col_index - 1);
			}
			else
			{
				std::stringstream message;
				message << "'col_index' is wrong ('col_index' = " << col_index << ").";
				throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
			}
		}
	}

	// TODO: this verif. is time consuming
	//	for (unsigned i = 0; i < n_cols; i++)
	//	{
	//		getline(stream, line);
	//		values = split(line);
	//
	//		if (values.size() < cols_degree[i])
	//		{
	//			std::stringstream message;
	//			message << "'values.size()' has to be greater or equal to 'cols_degree[i]' ('values.size()' = "
	//			        << values.size() << ", 'i' = " << i << ", 'cols_degree[i]' = " << cols_degree[i] << ").";
	//			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	//		}
	//
	//		for (unsigned j = 0; j < cols_max_degree; j++)
	//		{
	//			auto row_index = (j < values.size()) ? std::stoi(values[j]) : 0;
	//			if ((row_index >  0 && j <  cols_degree[i]) ||
	//				(row_index <= 0 && j >= cols_degree[i]))
	//			{
	//				if (row_index)
	//				{
	//					try
	//					{
	//						matrix.add_connection(row_index -1, i);
	//						throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ "The input AList file is not consistent.");
	//					}
	//					catch (std::exception const&)
	//					{
	//						// everything is normal if the code passes through the catch
	//					}
	//				}
	//			}
	//			else
	//			{
	//				std::stringstream message;
	//				message << "'row_index' is wrong ('row_index' = " << row_index << ").";
	//				throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	//			}
	//		}
	//	}

	return matrix;
}

// format ANR NAND
Sparse_matrix AList ::read_format3(std::istream &stream)
{
	std::string line;

	getline(stream, line);
	auto values = split(line);
	if (values.size() < 2)
	{
		std::stringstream message;
		message << "'values.size()' has to be greater than 1 ('values.size()' = " << values.size() << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	unsigned n_rows = 0, n_cols = 0;

	n_cols = std::stoi(values[0]);
	n_rows = std::stoi(values[1]);

	if (n_rows == 0 || n_cols == 0)
	{
		std::stringstream message;
		message << "'n_rows' and 'n_cols' have to be greater than 0 ('n_rows' = " << n_rows
				<< ", 'n_cols' = " << n_cols << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	Sparse_matrix matrix(n_rows, n_cols);

	getline(stream, line);
	values = split(line);
	if (values.size() < n_rows)
	{
		std::stringstream message;
		message << "'values.size()' has to be greater or equal to 'n_rows' ('values.size()' = " << values.size()
				<< ", 'n_rows' = " << n_rows << ").";
		throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
	}

	std::vector<unsigned> cols_degree(n_cols);
	for (unsigned i = 0; i < n_cols; i++)
	{
		unsigned n_connections = std::stoi(values[i]);
		if (n_connections > 0 && n_connections <= n_rows)
			cols_degree[i] = n_connections;
		else
		{
			std::stringstream message;
			message << "'n_connections' has to be greater than 0 and smaller than 'n_rows' "
					<< "('n_connections' = " << n_connections << ", 'n_rows' = " << n_rows << ").";
			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		}
	}

	for (unsigned i = 0; i < n_cols; i++)
	{
		getline(stream, line);
		values = split(line);

		if (values.size() < cols_degree[i])
		{
			std::stringstream message;
			message << "'values.size()' has to be greater or equal to 'cols_degree[i]' ('values.size()' = "
					<< values.size() << ", 'i' = " << i << ", 'cols_degree[i]' = " << cols_degree[i] << ").";
			throw std::runtime_error(/*__FILE__, __LINE__, __func__,*/ message.str());
		}

		for (unsigned j = 0; j < cols_degree[i]; j++)
		{
			unsigned row_index = std::stoi(values[j]);
			matrix.add_connection(row_index, i);
		}
	}

	return matrix;
}
