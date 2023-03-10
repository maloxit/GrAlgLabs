#include "utils.h"

#include <vector>


std::wstring Extension(const std::wstring& path)
{
    std::string::size_type idx;

    idx = path.rfind(L'.');

    if (idx != std::wstring::npos)
    {
        return path.substr(idx + 1);
    }
    else
    {
        return L"";
    }
}

std::string WCSToMBS(const std::wstring& wstr)
{
    constexpr size_t BUFFER_SIZE = 250;
    std::vector<char> buff;
    buff.resize(BUFFER_SIZE + 1);
    size_t size;
    wcstombs_s(&size, buff.data(), BUFFER_SIZE + 1, wstr.c_str(), BUFFER_SIZE);
    return std::string(buff.begin(), buff.end());
}
