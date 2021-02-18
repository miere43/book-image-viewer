#include "common.hpp"
#include <stdio.h>
#include <stdlib.h>

extern HWND hwnd;

void verifyImpl(const char* msg, const char* file, int line) {
    char buffer[1024];
    sprintf_s(buffer, "Assertion failed at %s(%d): %s", file, line, msg);
    MessageBoxA(hwnd, buffer, "Assertion failed", MB_ICONERROR);
    exit(1);
}
