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
#include <cstdio>

int main() {
    // 创建游戏窗口实例：800x650分辨率，60FPS
    GameWindow game(800, 650, 60);

    // 初始化游戏（创建窗口、加载谱面）
    if (!game.init()) {
        printf("游戏初始化失败！\n");
        return -1;
    }

    // 运行游戏主循环（阻塞直到游戏结束）
    game.run();

    return 0;
}
