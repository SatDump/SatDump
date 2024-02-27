#pragma once

#include <cstdint>
#include <vector>

namespace meteor
{
	namespace msumr
	{
		namespace lrpt
		{
			struct dc_t
			{
				std::vector<bool> code;
				int len;
			};

			struct ac_t
			{
				std::vector<bool> code;
				int clen;
				int zlen;
			};

			const float qTable[64] = {
				16, 11, 10, 16, 24, 40, 51, 61,		//
				12, 12, 14, 19, 26, 58, 60, 55,		//
				14, 13, 16, 24, 40, 57, 69, 56,		//
				14, 17, 22, 29, 51, 87, 80, 62,		//
				18, 22, 37, 56, 68, 109, 103, 77,	//
				24, 35, 55, 64, 81, 104, 113, 92,	//
				49, 64, 78, 87, 103, 121, 120, 101, //
				72, 92, 95, 98, 112, 100, 103, 99	//
			};

			const int64_t Zigzag[64] = {
				0, 1, 5, 6, 14, 15, 27, 28,		//
				2, 4, 7, 13, 16, 26, 29, 42,	//
				3, 8, 12, 17, 25, 30, 41, 43,	//
				9, 11, 18, 24, 31, 40, 44, 53,	//
				10, 19, 23, 32, 39, 45, 52, 54, //
				20, 22, 33, 38, 46, 51, 55, 60, //
				21, 34, 37, 47, 50, 56, 59, 61, //
				35, 36, 48, 49, 57, 58, 62, 63	//
			};

			const dc_t dcCategories[12] = {
				{{false, false}, 0},
				{{false, true, false}, 1},
				{{false, true, true}, 2},
				{{true, false, false}, 3},
				{{true, false, true}, 4},
				{{true, true, false}, 5},
				{{true, true, true, false}, 6},
				{{true, true, true, true, false}, 7},
				{{true, true, true, true, true, false}, 8},
				{{true, true, true, true, true, true, false}, 9},
				{{true, true, true, true, true, true, true, false}, 10},
				{{true, true, true, true, true, true, true, true, false}, 11},
			};

			const ac_t acCategories[162] = {
				{{true, false, true, false}, 0, 0},
				{{false, false}, 1, 0},
				{{false, true}, 2, 0},
				{{true, false, false}, 3, 0},
				{{true, false, true, true}, 4, 0},
				{{true, true, false, true, false}, 5, 0},
				{{true, true, true, true, false, false, false}, 6, 0},
				{{true, true, true, true, true, false, false, false}, 7, 0},
				{{true, true, true, true, true, true, false, true, true, false}, 8, 0},
				{{true, true, true, true, true, true, true, true, true, false, false, false, false, false, true, false}, 9, 0},
				{{true, true, true, true, true, true, true, true, true, false, false, false, false, false, true, true}, 10, 0},
				{{true, true, false, false}, 1, 1},
				{{true, true, false, true, true}, 2, 1},
				{{true, true, true, true, false, false, true}, 3, 1},
				{{true, true, true, true, true, false, true, true, false}, 4, 1},
				{{true, true, true, true, true, true, true, false, true, true, false}, 5, 1},
				{{true, true, true, true, true, true, true, true, true, false, false, false, false, true, false, false}, 6, 1},
				{{true, true, true, true, true, true, true, true, true, false, false, false, false, true, false, true}, 7, 1},
				{{true, true, true, true, true, true, true, true, true, false, false, false, false, true, true, false}, 8, 1},
				{{true, true, true, true, true, true, true, true, true, false, false, false, false, true, true, true}, 9, 1},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, false, false, false}, 10, 1},
				{{true, true, true, false, false}, 1, 2},
				{{true, true, true, true, true, false, false, true}, 2, 2},
				{{true, true, true, true, true, true, false, true, true, true}, 3, 2},
				{{true, true, true, true, true, true, true, true, false, true, false, false}, 4, 2},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, false, false, true}, 5, 2},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, false, true, false}, 6, 2},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, false, true, true}, 7, 2},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, true, false, false}, 8, 2},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, true, false, true}, 9, 2},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, true, true, false}, 10, 2},
				{{true, true, true, false, true, false}, 1, 3},
				{{true, true, true, true, true, false, true, true, true}, 2, 3},
				{{true, true, true, true, true, true, true, true, false, true, false, true}, 3, 3},
				{{true, true, true, true, true, true, true, true, true, false, false, false, true, true, true, true}, 4, 3},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, false, false, false}, 5, 3},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, false, false, true}, 6, 3},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, false, true, false}, 7, 3},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, false, true, true}, 8, 3},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, true, false, false}, 9, 3},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, true, false, true}, 10, 3},
				{{true, true, true, false, true, true}, 1, 4},
				{{true, true, true, true, true, true, true, false, false, false}, 2, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, true, true, false}, 3, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, false, true, true, true}, 4, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, false, false, false}, 5, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, false, false, true}, 6, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, false, true, false}, 7, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, false, true, true}, 8, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, true, false, false}, 9, 4},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, true, false, true}, 10, 4},
				{{true, true, true, true, false, true, false}, 1, 5},
				{{true, true, true, true, true, true, true, false, true, true, true}, 2, 5},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, true, true, false}, 3, 5},
				{{true, true, true, true, true, true, true, true, true, false, false, true, true, true, true, true}, 4, 5},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, false, false, false}, 5, 5},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, false, false, true}, 6, 5},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, false, true, false}, 7, 5},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, false, true, true}, 8, 5},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, true, false, false}, 9, 5},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, true, false, true}, 10, 5},
				{{true, true, true, true, false, true, true}, 1, 6},
				{{true, true, true, true, true, true, true, true, false, true, true, false}, 2, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, true, true, false}, 3, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, false, true, true, true}, 4, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, false, false, false}, 5, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, false, false, true}, 6, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, false, true, false}, 7, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, false, true, true}, 8, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, true, false, false}, 9, 6},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, true, false, true}, 10, 6},
				{{true, true, true, true, true, false, true, false}, 1, 7},
				{{true, true, true, true, true, true, true, true, false, true, true, true}, 2, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, true, true, false}, 3, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, false, true, true, true, true}, 4, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, false, false, false}, 5, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, false, false, true}, 6, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, false, true, false}, 7, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, false, true, true}, 8, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, true, false, false}, 9, 7},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, true, false, true}, 10, 7},
				{{true, true, true, true, true, true, false, false, false}, 1, 8},
				{{true, true, true, true, true, true, true, true, true, false, false, false, false, false, false}, 2, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, true, true, false}, 3, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, false, true, true, true}, 4, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, false, false, false}, 5, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, false, false, true}, 6, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, false, true, false}, 7, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, false, true, true}, 8, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, true, false, false}, 9, 8},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, true, false, true}, 10, 8},
				{{true, true, true, true, true, true, false, false, true}, 1, 9},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, true, true, false}, 2, 9},
				{{true, true, true, true, true, true, true, true, true, false, true, true, true, true, true, true}, 3, 9},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false}, 4, 9},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, true}, 5, 9},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, false, true, false}, 6, 9},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, false, true, true}, 7, 9},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, true, false, false}, 8, 9},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, true, false, true}, 9, 9},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, true, true, false}, 10, 9},
				{{true, true, true, true, true, true, false, true, false}, 1, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, false, true, true, true}, 2, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, false, false, false}, 3, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, false, false, true}, 4, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, false, true, false}, 5, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, false, true, true}, 6, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, true, false, false}, 7, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, true, false, true}, 8, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, true, true, false}, 9, 10},
				{{true, true, true, true, true, true, true, true, true, true, false, false, true, true, true, true}, 10, 10},
				{{true, true, true, true, true, true, true, false, false, true}, 1, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, false, false, false}, 2, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, false, false, true}, 3, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, false, true, false}, 4, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, false, true, true}, 5, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, true, false, false}, 6, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, true, false, true}, 7, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, true, true, false}, 8, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, false, true, true, true}, 9, 11},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, false, false, false}, 10, 11},
				{{true, true, true, true, true, true, true, false, true, false}, 1, 12},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, false, false, true}, 2, 12},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, false, true, false}, 3, 12},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, false, true, true}, 4, 12},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, true, false, false}, 5, 12},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, true, false, true}, 6, 12},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, true, true, false}, 7, 12},
				{{true, true, true, true, true, true, true, true, true, true, false, true, true, true, true, true}, 8, 12},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false}, 9, 12},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, true}, 10, 12},
				{{true, true, true, true, true, true, true, true, false, false, false}, 1, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, false, true, false}, 2, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, false, true, true}, 3, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, true, false, false}, 4, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, true, false, true}, 5, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, true, true, false}, 6, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, false, true, true, true}, 7, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, false, false, false}, 8, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, false, false, true}, 9, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, false, true, false}, 10, 13},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, false, true, true}, 1, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, true, false, false}, 2, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, true, false, true}, 3, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, true, true, false}, 4, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, false, true, true, true, true}, 5, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false}, 6, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, true}, 7, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, false, true, false}, 8, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, false, true, true}, 9, 14},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, true, false, false}, 10, 14},
				{{true, true, true, true, true, true, true, true, false, false, true}, 0, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, true, false, true}, 1, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, true, true, false}, 2, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, false, true, true, true}, 3, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false}, 4, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, true}, 5, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, true, false, true, false}, 6, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, true, false, true, true}, 7, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false}, 8, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, true}, 9, 15},
				{{true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false}, 10, 15},
			};
		} // namespace lrpt
	}	  // namespace msumr
} // namespace meteor