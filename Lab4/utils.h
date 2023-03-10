#pragma once

#include <string>

#define SAFE_RELEASE(A) if ((A) != NULL) { (A)->Release(); (A) = NULL; }

inline unsigned int DivUp(unsigned int a, unsigned int b)
{
    return (a + b - 1) / b;
}

std::wstring Extension(const std::wstring& path);

std::string WCSToMBS(const std::wstring& wstr);
