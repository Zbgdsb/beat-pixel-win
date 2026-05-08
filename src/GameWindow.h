/**
 * @file GameWindow.h
 * @brief GameWindow游戏窗口类定义
 * @details 职责：窗口初始化、60FPS帧率控制、事件循环、全局渲染调度、按键输入处理
 *          管理4条轨道、计分系统、音频系统、谱面数据的完整游戏生命周期
 *          支持游戏状态切换：菜单 → 游戏中 → 结算界面
 *
 *          V2.0新增：
 *          - 主菜单界面（难度选择、歌曲选择）
 *          - 本地MP3导入 + 自动节拍解析
 *          - Combo加成倍率显示
 *          - 本地排行榜持久化
 *          - 纹理资源管理系统（TextureManager）
 *          - 动画系统（打击效果、判定文字、Combo跳动）
 */
#pragma once
#include "graphics.h"
#include <vector>
#include <utility>
#include <memory>
#include <string>
#include "NoteTrack.h"
#include "ScoreSystem.h"
#include "AudioManager.h"
#include "TextureManager.h"
#include "Animations.h"
#include "BeatParser.h"
#include "DataManager.h"

/**
 * @brief 游戏状态枚举
 */
enum GameState {
    MENU,       // 主菜单（V2.0新增）
    PLAYING,    // 游戏中
    RESULT      // 结算界面
};

/**
 * @brief 难度等级枚举（V2.0新增）
 */
enum Difficulty {
    EASY = 0,   // 简单：200px/s
    NORMAL = 1, // 普通：300px/s
    HARD = 2    // 困难：400px/s
};

class GameWindow {
private:
    int width;
    int height;
    int fps;

    std::vector<std::unique_ptr<NoteTrack>> tracks;
    ScoreSystem scoreSystem;
    AudioManager audioManager;

    // ========== V2.0新增：节拍解析与数据管理 ==========
    BeatParser beatParser;              // 节拍解析器
    DataManager dataManager;            // 数据管理器（排行榜）
    std::string currentSongName;        // 当前歌曲名称
    Difficulty currentDifficulty;       // 当前难度
    BeatParser::ParseResult parseResult; // 解析结果

    // ========== 纹理资源管理 ==========
    TextureManager textures;

    // ========== 动画系统 ==========
    std::vector<HitAnim> hitAnims;
    std::vector<TextPopupAnim> textAnims;
    ComboAnim comboAnim;
    int prevCombo = 0;

    // ========== 按键状态管理 ==========
    bool keyPressed[4] = {false, false, false, false};
    bool keyWasPressed[4] = {false, false, false, false};
    int keyGlowAlpha[4] = {0, 0, 0, 0};

    // ========== 菜单状态（V2.0新增） ==========
    int menuSelection = 0;          // 菜单选项索引
    int difficultySelection = 1;    // 难度选择（默认Normal）
    bool keyMenuWasPressed = false; // 菜单按键防重复
    bool importRequested = false;   // 是否请求导入MP3

    GameState gameState;
    long long gameStartTime;
    long long currentTime;

    bool isRunning;

    Judgement lastJudgement;
    int judgementDisplayTimer;

    // 结算界面
    long long resultStartTime;

    // 谱面数据
    std::vector<std::pair<long long, int>> noteTimeData;
    long long lastNoteTime;

    static const int TRACK_COUNT = 4;
    static const int TRACK_WIDTH = 100;
    static const int JUDGE_Y = 500;
    static const char TRACK_KEYS[4];
    static const COLORREF TRACK_COLORS[4];

    // 难度对应的下落速度
    static const double DIFFICULTY_SPEEDS[3];

    void initTracks();
    void loadDemoSong();
    bool loadSongFromMP3(const std::string& filePath);
    void handleInput();
    void handleResultInput();
    void handleMenuInput();
    void update();
    void updateAnimations();
    void spawnHitAnim(int trackId, float noteY, Judgement j);
    void spawnTextPopup(int trackId, Judgement j);
    void render();
    void drawUI();
    void drawResultScreen();
    void drawMenuScreen();
    void drawDynamicBackground();  // 动态背景
    void drawAnimations();
    int getTrackX(int trackId) const;
    bool isGameFinished() const;
    const char* getRating() const;
    COLORREF getRatingColor() const;

    /**
     * @brief 获取难度对应的下落速度
     */
    double getDifficultySpeed() const;

    /**
     * @brief 获取难度名称
     */
    const char* getDifficultyName() const;

public:
    GameWindow(int width = 800, int height = 650, int fps = 60);
    ~GameWindow() = default;

    bool init();
    void run();
};
