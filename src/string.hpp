#pragma once
#include "common.hpp"
#include <stdarg.h>

String toUtf8(const wchar_t* src, size_t src_length);
wchar_t* toUtf16(const String& src, int* dst_length);
String copyString(const String& str);
char* toCString(const String& str);
bool stringEqualsCaseInsensitive(const String& a, const String& b);
bool stringEqualsCaseInsensitive(const char* a, size_t a_length, const char* b, size_t b_length);
bool stringEquals(const String& a, const String& b);
bool stringEquals(const char* a, size_t a_length, const char* b, size_t b_length);
inline bool operator==(const String& a, const String& b) { return stringEquals(a, b); }
inline bool operator!=(const String& a, const String& b) { return !stringEquals(a, b); }
inline bool operator==(const String& a, const char* b) { return stringEquals(a.chars, a.count, b, b ? strlen(b) : 0); }
inline bool operator!=(const String& a, const char* b) { return !stringEquals(a.chars, a.count, b, b ? strlen(b) : 0); }
String mprintf(const char* format, ...);
String mprintf_valist(const char* format, va_list args);
bool tryParseInt(const String& value, int* result);
int parseInt(const String& value);
bool parseBoolean(const String& value);
String substring(const String& str, int start, int count);
int indexOf(const String& str, char c);
int lastIndexOf(const String& str, char c);
