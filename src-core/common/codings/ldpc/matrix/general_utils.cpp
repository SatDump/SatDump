#include <map>
#include <cmath>
#include <limits>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <iomanip>

// #include "Tools/Math/utils.h"
// #include "Tools/Exception/exception.hpp"
#include "general_utils.h"

std::vector<std::string> codings::ldpc::split(const std::string &s, char delim)
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;
	while (std::getline(ss, item, delim))
		elems.push_back(std::move(item));

	return elems;
}

std::vector<std::string> codings::ldpc::split(const std::string &s)
{
	std::string buf;				 // have a buffer string
	std::stringstream ss(s);		 // insert the string into a stream
	std::vector<std::string> tokens; // create vector to hold our words

	while (ss >> buf)
		tokens.push_back(buf);

	return tokens;
}

void codings::ldpc::getline(std::istream &file, std::string &line)
{
	if (file.eof() || file.fail() || file.bad())
		throw std::runtime_error("Something went wrong when getting a new line."); // runtime_error(__FILE__, __LINE__, __func__, "Something went wrong when getting a new line.");

	while (std::getline(file, line))
		if (line[0] != '#' && !std::all_of(line.begin(), line.end(), isspace))
			break;
}
