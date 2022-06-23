#pragma once
#include <string>

inline bool FindStringInBetween(std::wstring& dst, const std::wstring& src, const wchar_t* token_begin, const wchar_t* token_end) {
    auto p1 = src.find(token_begin);
    if (p1 == std::wstring::npos) return false;
    auto offset_begin = p1 + wcslen(token_begin);
    auto p2 = src.find(token_end, offset_begin);
    if (p2 == std::wstring::npos) return false;
    auto offset_end = p2;

    dst = src.substr(offset_begin, offset_end - offset_begin);
    return true;
}
