#include "string.h"
#include <algorithm>
#include <codecvt>
#include <fstream>
#include <locale>
#include <sstream>

namespace satdump
{
    std::vector<std::string> splitString(std::string input, char del)
    {
        std::stringstream stcStream(input);
        std::string seg;
        std::vector<std::string> segs;

        while (std::getline(stcStream, seg, del))
            segs.push_back(seg);

        return segs;
    }

    void replaceAllStr(std::string &str, const std::string &from, const std::string &to)
    {
        if (from.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }

    bool isStringPresent(std::string searched, std::string keyword)
    {
        std::transform(searched.begin(), searched.end(), searched.begin(), tolower);
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), tolower);

        auto found_it = searched.find(keyword, 0);
        return found_it != std::string::npos;
    }

    std::string loadFileToString(std::string path)
    {
        std::ifstream f(path);
        std::string str = std::string(std::istreambuf_iterator<char>{f}, {});
        f.close();
        return str;
    }

    std::string ws2s(const std::wstring &wstr)
    {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;
        return converterX.to_bytes(wstr);
    }

    std::wstring s2ws(const std::string &str)
    {
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;
        return converterX.from_bytes(str);
    }
} // namespace satdump