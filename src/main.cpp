/**
 * @file main.cpp
 * @brief 《像素节拍》4键下落式音游 - 主函数入口
 * @details 程序启动流程：创建游戏窗口 -> 初始化 -> 运行游戏主循环 -> 结束
 *
 * 游戏简介：
 *   《像素节拍》是一款基于EasyX图形库的4键下落式音游
 *   玩家通过A/S/D/F四个按键，在音符到达判定线时准确按下对应按键
 *   系统根据按键时机给出Perfect/Good/Miss三级判定，累计得分和Combo
 *
 * 操作说明：
 *   A - 轨道1（最左侧）
 *   S - 轨道2
 *   D - 轨道3
 *   F - 轨道4（最右侧）
 *   ESC - 退出游戏
 *
 * 判定规则：
 *   Perfect: 按键时间与音符判定时间差 ≤ 50ms，得100分 + Combo加成
 *   Good:    按键时间与音符判定时间差 ≤ 150ms，得50分 + Combo加成
 *   Miss:    超过150ms未按键或按键时机过早，Combo重置
 *
 * @author BeatPixel Team
 * @date 2026
 */
#include "GameWindow.h"
#include "DebugLog.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>

// V4.0: 未处理异常捕获 - 写入 crash.log 辅助诊断闪退
static LONG WINAPI BeatPixelCrashHandler(EXCEPTION_POINTERS* info) {
    FILE* f = fopen("crash.log", "w");
    if (f) {
        fprintf(f, "Exception code: 0x%08X\n", (unsigned)info->ExceptionRecord->ExceptionCode);
        fprintf(f, "Fault address: 0x%p\n", info->ExceptionRecord->ExceptionAddress);
        fclose(f);
    }
    MessageBoxA(NULL, "BeatPixel 发生错误，详情已写入 crash.log", "错误", MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main() {
#ifdef _WIN32
    // 安装崩溃捕获
    SetUnhandledExceptionFilter(BeatPixelCrashHandler);
    debugLog("main: start");
#endif

    // 创建游戏窗口实例：800x650分辨率，60FPS
    GameWindow game(900, 650, 60);

    // 初始化游戏（创建窗口、加载谱面）
    debugLog("main: calling init");
    if (!game.init()) {
        printf("游戏初始化失败！\n");
        debugLog("main: init failed");
        return -1;
    }
    debugLog("main: init ok");

    // 运行游戏主循环（阻塞直到游戏结束）
    game.run();

    debugLog("main: exit");
    return 0;
}
