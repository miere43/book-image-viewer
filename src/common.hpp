#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

__declspec(noreturn) void verifyImpl(const char* msg, const char* file, int line);

#ifdef _DEBUG
#define verify(cond) do { if (!(cond)) __debugbreak(); } while (0)
#else
#define verify(cond) do { if (!(cond)) verifyImpl(#cond, __FILE__, __LINE__); } while (0)
#endif

struct String {
    char* chars;
    int count;

    inline void destroy() { delete[] chars; chars = nullptr; count = 0; }
    inline bool isEmpty() const { return chars == nullptr || count <= 0; }
    inline char operator[](size_t i) const { return chars[i]; }

    inline String(char* chars, int count) : chars(chars), count(count) { }
    inline String() : chars(nullptr), count(0) { }
    template<size_t N> constexpr String(const char(&a)[N]) : chars((char*)a), count(N - 1) { }
};

inline String wrapCString(const char* str) {
    return String{ (char*)str, str ? (int)strlen(str) : 0 };
}
