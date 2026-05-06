/**
 * @file GameWindow.h
 * @brief GameWindow游戏窗口类定义
 * @details 职责：窗口初始化、60FPS帧率控制、事件循环、全局渲染调度、按键输入处理
 *          管理4条轨道、计分系统、音频系统、谱面数据的完整游戏生命周期
 *          支持游戏状态切换：游戏中 -> 结算界面
 */
#pragma once
#include "graphics.h"
#include <vector>
#include <utility>
#include <memory>
#include "NoteTrack.h"
#include "ScoreSystem.h"
#include "AudioManager.h"

enum GameState {
    PLAYING,
    RESULT
};

class GameWindow {
private:
    int width;
    int height;
    int fps;

    std::vector<std::unique_ptr<NoteTrack>> tracks;
    ScoreSystem scoreSystem;
    AudioManager audioManager;

    GameState gameState;
    long long gameStartTime;
    long long currentTime;
    bool isRunning;

    Judgement lastJudgement;
    int judgementDisplayTimer;

    // 结算界面
    long long resultStartTime;

    // 谱面数据（供BGM同步生成）
    std::vector<std::pair<long long, int>> noteTimeData; // (时间戳ms, 轨道编号)
    long long lastNoteTime; // 最后一个音符的时间（用于游戏结束判定）

    static const int TRACK_COUNT = 4;
    static const int TRACK_WIDTH = 100;
    static const int JUDGE_Y = 500;
    static const double NOTE_SPEED;
    static const char TRACK_KEYS[4];
    static const COLORREF TRACK_COLORS[4];

    void initTracks();
    void loadDemoSong();
    void handleInput();
    void handleResultInput();
    void update();
    void render();
    void drawUI();
    void drawResultScreen();
    int getTrackX(int trackId) const;
    bool isGameFinished() const; // 基于时间判定游戏结束
    const char* getRating() const;
    COLORREF getRatingColor() const;

public:
    GameWindow(int width = 800, int height = 650, int fps = 60);
    ~GameWindow() = default;

    bool init();
    void run();
};
