#include "string.hpp"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

String toUtf8(const wchar_t* src, size_t src_length) {
    if (!src) return {};

    int bytes_required = WideCharToMultiByte(CP_UTF8, 0, src, (int)src_length, nullptr, 0, nullptr, nullptr);
    verify(bytes_required > 0);

    char* dst = new char[bytes_required + 1];

    int chars_written = WideCharToMultiByte(CP_UTF8, 0, src, (int)src_length, dst, bytes_required, nullptr, nullptr);
    verify(chars_written > 0);
    verify(chars_written <= bytes_required);

    dst[chars_written] = '\0';
    return { dst, chars_written };
}

wchar_t* toUtf16(const String& src, int* dst_length) {
    if (!src.chars) return nullptr;

    int chars_required = MultiByteToWideChar(CP_UTF8, 0, src.chars, (int)src.count, nullptr, 0);
    verify(chars_required >= 0);

    wchar_t* dst = new wchar_t[chars_required + 1];

    int chars_written = MultiByteToWideChar(CP_UTF8, 0, src.chars, (int)src.count, dst, chars_required);
    verify(chars_written >= 0);
    verify(chars_written <= chars_required);

    dst[chars_written] = L'\0';

    if (dst_length) *dst_length = chars_written;
    return dst;
}

String mprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    auto result = mprintf_valist(format, args);
    va_end(args);
    return result;
}

String mprintf_valist(const char* format, va_list args) {
    int count = _vscprintf(format, args);
    verify(count >= 0);

    auto result = new char[count + 1];
    int realCount = vsnprintf(result, count + 1, format, args);
    verify(realCount >= 0);
    return String{ result, realCount };
}

String copyString(const String& str) {
    if (str.isEmpty()) return {};
    auto chars = new char[str.count];
    memcpy(chars, str.chars, str.count);
    return { chars, str.count };
}

char* toCString(const String& str) {
    auto result = new char[str.count + 1];
    memcpy(result, str.chars, str.count);
    result[str.count] = '\0';
    return result;
}

bool stringEqualsCaseInsensitive(const String& a, const String& b) {
    return stringEqualsCaseInsensitive(a.chars, a.count, b.chars, b.count);
}

bool stringEqualsCaseInsensitive(const char* a, size_t a_length, const char* b, size_t b_length) {
    if (a_length != b_length) return false;
    if (a == b) return true;
    return _strnicmp(a, b, a_length) == 0;
}

bool stringEquals(const String& a, const String& b) {
    return stringEquals(a.chars, a.count, b.chars, b.count);
}

bool stringEquals(const char* a, size_t a_length, const char* b, size_t b_length) {
    if (a_length != b_length) return false;
    if (a == b) return true;
    return strncmp(a, b, a_length) == 0;
}

bool tryParseInt(const String& value, int* outResult) {
    if (value.count > 10 || value.isEmpty()) {
        return false;
    }

    auto now = value.chars;
    auto end = value.chars + value.count;

    bool is_positive = true;
    switch (now[0]) {
        case '-': is_positive = false; ++now; break;
        case '+': is_positive = true; ++now; break;
    }

    if (now == end) {
        return false;
    }

    int64_t result = 0;
    while (now < end) {
        char c = *now++;
        if (c >= '0' && c <= '9') {
            int digit = c - '0';
            result = result * 10 + digit;
        } else {
            return false;
        }
    }

    if (!is_positive) {
        result = -result;
    }
    if (result < INT_MIN) return false;
    if (result > INT_MAX) return false;

    *outResult = static_cast<int>(result);
    return true;
}

int parseInt(const String& value) {
    int result;
    verify(tryParseInt(value, &result));
    return result;
}

bool parseBoolean(const String& value) {
    if (stringEqualsCaseInsensitive(value, "True")) {
        return true;
    } else if (stringEqualsCaseInsensitive(value, "False")) {
        return false;
    }
    verify(false);
    return false;
}

String substring(const String& str, int start, int count) {
    verify(start >= 0);
    verify(count >= 0 && count <= str.count);
    return { str.chars + start, count };
}

int indexOf(const String& str, char c) {
    for (int i = 0; i < str.count; ++i) {
        if (str.chars[i] == c) {
            return i;
        }
    }
    return -1;
}

int lastIndexOf(const String& str, char c) {
    for (int i = str.count - 1; i >= 0; --i) {
        if (str.chars[i] == c) {
            return i;
        }
    }
    return -1;
}
