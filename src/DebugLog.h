/**
 * @file DebugLog.h
 * @brief V4.0: 跨平台调试日志 - 每次 fclose 确保闪退时最后一行可读
 */
#pragma once
#include <cstdio>

inline void debugLog(const char* msg) {
    FILE* f = fopen("beatpixel_debug.log", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}
