#pragma once

/**
 * @file string.h
 */

#include <string>
#include <vector>

namespace satdump
{
    /**
     * @brief Split a string into multiples where
     * a specific char is present.
     *
     * @param input the string to split
     * @param del delimiter character
     * @return string segments
     */
    std::vector<std::string> splitString(std::string input, char del);

    /**
     * @brief Replaces all occurences of a string in
     * a string with another string. (lots of strings!)
     *
     * @param str the string to replace in
     * @param from pattern to replace
     * @param to pattern to replace the old pattern with
     */
    void replaceAllStr(std::string &str, const std::string &from, const std::string &to);

    /**
     * @brief Check if a pattern is present in the
     * provided string. Case-insensitive
     *
     * @param searched string to search in
     * @param keyword pattern to search
     * @return true if present
     */
    bool isStringPresent(std::string searched, std::string keyword);

    /**
     * @brief Load a text file into a std::string
     *
     * @param path file path
     * @return loaded string
     */
    std::string loadFileToString(std::string path);

    /**
     * @brief Convert a std::wstring to std::string
     *
     * @param wstr input std::wstring
     * @return output std::string
     */
    std::string ws2s(const std::wstring &wstr);

    /**
     * @brief Convert a std::string to std::wstring
     *
     * @param wstr input std::string
     * @return output std::wstring
     */
    std::wstring s2ws(const std::string &str);
} // namespace satdump