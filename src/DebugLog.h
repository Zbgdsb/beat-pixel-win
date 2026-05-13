/**
 * @file DebugLog.h
 * @brief V4.0: 跨平台调试日志 - 每次 fclose 确保闪退时最后一行可读
 */
#pragma once
#include <cstdio>
#include <ctime>

inline void debugLog(const char* msg) {
    FILE* f = fopen("beatpixel_debug.log", "a");
    if (f) {
        time_t now = time(nullptr);
        struct tm t;
#ifdef _WIN32
        localtime_s(&t, &now);
#else
        localtime_r(&now, &t);
#endif
        fprintf(f, "[%02d:%02d:%02d] %s\n", t.tm_hour, t.tm_min, t.tm_sec, msg);
        fclose(f);
    }
}
