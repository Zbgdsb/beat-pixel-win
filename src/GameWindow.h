/**
 * @file GameWindow.h
 * @brief GameWindow游戏窗口类定义
 * @details 职责：窗口初始化、60FPS帧率控制、事件循环、全局渲染调度、按键输入处理
 *          管理4条轨道、计分系统、谱面数据的完整游戏生命周期
 *          支持游戏状态切换：游戏中 -> 结算界面
 */
#pragma once
#include "graphics.h"
#include <vector>
#include <memory>
#include "NoteTrack.h"
#include "ScoreSystem.h"
#include "AudioManager.h"

/**
 * @brief 游戏状态枚举
 */
enum GameState {
    PLAYING,    // 游戏进行中
    RESULT      // 结算界面
};

/**
 * @brief 游戏主窗口类
 * @details 控制整个游戏的生命周期：初始化 -> 游戏循环 -> 结算 -> 结束
 *          游戏循环内按固定60FPS执行：输入处理 -> 状态更新 -> 画面渲染
 */
class GameWindow {
private:
    // 窗口参数
    int width;          // 窗口宽度(px)
    int height;         // 窗口高度(px)
    int fps;            // 目标帧率

    // 游戏核心组件
    std::vector<std::unique_ptr<NoteTrack>> tracks; // 4条轨道
    ScoreSystem scoreSystem;                         // 计分系统
    AudioManager audioManager;                       // 音频管理器

    // 游戏状态
    GameState gameState;       // 当前游戏状态
    long long gameStartTime;  // 游戏开始时间戳(ms)
    long long currentTime;    // 当前游戏内时间(ms)
    bool isRunning;           // 游戏是否正在运行

    // 判定结果显示（用于UI闪烁）
    Judgement lastJudgement;  // 最近一次判定结果
    int judgementDisplayTimer;// 判定结果显示计时器(帧数)

    // 结算界面
    long long resultStartTime; // 结算界面开始时间（用于淡入动画）

    // 轨道参数常量
    static const int TRACK_COUNT = 4;       // 轨道数量
    static const int TRACK_WIDTH = 100;     // 每条轨道宽度(px)
    static const int JUDGE_Y = 500;         // 判定线Y坐标
    static const double NOTE_SPEED;         // 音符下落速度(px/帧)

    // 轨道对应的按键
    static const char TRACK_KEYS[4];        // A, S, D, F

    // 轨道颜色（用于区分不同轨道的音符）
    static const COLORREF TRACK_COLORS[4];

    // 内部方法
    void initTracks();                      // 初始化4条轨道
    void loadDemoSong();                    // 加载演示谱面
    void handleInput();                     // 处理按键输入（游戏中状态）
    void handleResultInput();               // 处理按键输入（结算界面状态）
    void update();                          // 更新游戏状态
    void render();                          // 渲染画面（根据状态分发）
    void drawUI();                          // 绘制UI（分数、Combo、判定）
    void drawResultScreen();               // 绘制结算界面
    int getTrackX(int trackId) const;       // 计算轨道X坐标
    bool isAllNotesCleared() const;         // 检测所有音符是否已处理完毕
    const char* getRating() const;          // 根据Perfect率计算评级
    COLORREF getRatingColor() const;        // 获取评级对应颜色

public:
    GameWindow(int width = 800, int height = 650, int fps = 60);
    ~GameWindow() = default;

    /**
     * @brief 初始化游戏窗口
     * @details 创建EasyX图形窗口，初始化轨道、音频和计分系统，加载谱面
     * @return true初始化成功，false失败
     */
    bool init();

    /**
     * @brief 运行游戏主循环
     * @details 以60FPS循环执行：输入处理 -> 状态更新 -> 画面渲染
     *          游戏结束后进入结算界面，直到用户退出
     */
    void run();
};
