/*!
 * \file
 * \brief Struct tools::AList.
 */
#pragma once

#include <iostream>
#include <vector>

#include "sparse_matrix.h"

namespace codings
{
	namespace ldpc
	{
		struct AList
		{
		public:
			static Sparse_matrix read(std::istream &stream);												 // WARNING (Aang23) : If loading an H matrix, must turn to be horizontal!
			static void write(const Sparse_matrix &matrix, std::ostream &stream, bool zero_padding = false); // WARNING (Aang23) : If writing an H matrix, must turn to be vertical!

			/*
			 * get the matrix dimensions H and N from the input stream
			 * @H is the height of the matrix
			 * @N is the width of the matrix
			 */
			static void read_matrix_size(std::istream &stream, int &H, int &N);

			static std::vector<unsigned> read_info_bits_pos(std::istream &stream);
			static std::vector<unsigned> read_info_bits_pos(std::istream &stream, const int K, const int N);
			static void write_info_bits_pos(const std::vector<unsigned> &info_bits_pos, std::ostream &stream);

		private:
			static Sparse_matrix read_format1(std::istream &stream);
			static Sparse_matrix read_format2(std::istream &stream);
			static Sparse_matrix read_format3(std::istream &stream);
		};
	}
}
