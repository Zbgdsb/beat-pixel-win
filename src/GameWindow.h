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
#include "DebugLog.h"
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
#include "SongAnalyzer.h"
#include "ChartPackage.h"
#include "AchievementSystem.h"

/**
 * @brief 游戏状态枚举
 */
enum GameState {
    MENU,       // 主菜单（V2.0新增）
    PLAYING,    // 游戏中
    PAUSED,     // V3.1: 游戏暂停
    RESULT,     // 结算界面
    ANALYZING,  // V3.2: 歌曲分析界面
    TRACK_DELEGATE  // 代管轨道选择
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
    SongAnalyzer songAnalyzer;          // V3.2: 歌曲分析器
    AchievementSystem achievementSystem; // V3.3: 成就系统
    std::string currentSongName;        // 当前歌曲名称
    Difficulty currentDifficulty;       // 当前难度
    BeatParser::ParseResult parseResult; // 解析结果

    // ========== V3.2: 歌曲分析状态 ==========
    SongAnalyzer::AnalysisResult analysisResult; // 分析结果
    std::string analysisFilePath;       // 当前分析的文件路径
    int analysisMenuSelection = 0;      // 分析界面菜单选择 0:BPM 1:偏移 2:开始 3:导出 4:返回
    float manualBPM = 0;               // 手动BPM（0表示用自动检测的）
    float manualOffset = 0;            // 手动偏移(ms)
    float gameOffsetMs = 0.0f;         // V3.3: 游戏内实时偏移补偿(ms)
    bool analysisDone = false;          // 分析是否完成
    bool isInTapping = false;           // 是否在点拍模式

    // ========== V3.3: 成就弹窗 ==========
    struct AchievementPopup {
        std::string title;
        std::string description;
        std::string icon;
        float elapsed = 0.0f;
        static constexpr float DURATION = 3.0f;
    };
    std::vector<AchievementPopup> achievementPopups;
    int totalSongsPlayed = 0;
    std::string exeDir;

    // ========== 纹理资源管理 ==========
    TextureManager textures;

    // ========== 动画系统 ==========
    std::vector<HitAnim> hitAnims;
    std::vector<TextPopupAnim> textAnims;
    ComboAnim comboAnim;
    int prevCombo = 0;

    // ========== V3.0新增：判定粒子特效 ==========
    std::vector<Particle> particles;
    std::vector<MissCrossAnim> missCrossAnims;

    // ========== V3.0新增：按键视觉反馈 ==========
    KeyFeedback keyFeedback[6];

    // ========== V3.0新增：Combo断连闪烁 ==========
    float comboBreakFlash = 0.0f;   // Combo断掉时的红色闪烁剩余时间
    static constexpr float COMBO_BREAK_DURATION = 0.2f;  // 闪烁持续时间

    // ========== 按键状态管理 ==========
    bool keyPressed[6] = {false, false, false, false, false, false};
    bool keyWasPressed[6] = {false, false, false, false, false, false};
    int keyGlowAlpha[6] = {0, 0, 0, 0, 0, 0};

    // ========== 菜单状态（V2.0新增） ==========
    int menuSelection = 0;          // 菜单选项索引
    int difficultySelection = 1;    // 难度选择（默认Normal）
    bool importRequested = false;   // 是否请求导入MP3
    bool chartExportRequested = false; // V3.3: 导出谱面
    bool showAchievements = false;  // V3.3: 显示成就界面
    bool showTutorial = false;      // V3.4: 显示游玩说明界面

    // ========== V3.4: 歌曲列表 ==========
    struct SongListItem {
        std::string name;           // 歌曲名（文件名去掉后缀）
        std::string filePath;       // 完整文件路径
        bool hasChart;              // 是否已有谱面
    };
    std::vector<SongListItem> songList;
    int songListSelection = 0;     // 列表选中索引
    bool isShowingSongList = false; // 是否在歌曲列表界面
    bool songLoading = false;       // V4.0: 歌曲加载中（避免卡死错觉）
    bool songListJustOpened = false; // 防止Enter粘滞触发Demo
    bool settingsJustOpened = false;  // 防止Enter粘滞
    bool analysisJustOpened = false;  // 防止Enter粘滞（分析界面）
    bool trackDelegateJustOpened = false;  // 防止Enter/ESC粘滞（代管界面）
    bool pauseJustOpened = false;      // 防止ESC粘滞（暂停→立即恢复）
    bool menuJustOpened = false;        // 防止从子页面Enter返回时穿透
    bool autoPlay = false;           // V3.7: 自动演示模式
    bool trackAutoPlay[6] = {false}; // 代管轨道
    int trackDelegateSel = 0;        // 代管选轨光标
    float mouseX = 0, mouseY = 0;       // 当前鼠标坐标（悬浮高亮）
    void refreshSongList();         // 扫描songs目录生成列表
    bool loadChartFromFile(const std::string& chartPath, const std::string& mp3Path);  // 从.chart.json加载谱面
    void startSelectedSong(SongListItem& selected);  // 统一选歌启动入口

    GameState gameState;
    long long gameStartTime;
    long long currentTime;

    bool isRunning;

    Judgement lastJudgement;
    int judgementDisplayTimer;

    // 结算界面
    long long resultStartTime;

    // ========== V3.0: BPM指示器 ==========
    float currentBPM = 120.0f;        // 当前歌曲BPM
    float bpmPulsePhase = 0.0f;       // BPM脉冲相位 (0~2π)
    float bpmPulseScale = 1.0f;       // 当前脉冲缩放 (1.0~1.3)

    // ========== V3.0: 音符预读条 ==========
    static const float PREVIEW_DURATION; // 预览时长(秒)
    static const int PREVIEW_HEIGHT;     // 预读条高度
    static const int PREVIEW_Y;          // 预读条Y坐标

    // ========== V3.1: 暂停功能 ==========
    long long pauseStartTime = 0;        // 暂停开始时间(ms)
    int pauseMenuSelection = 0;          // 暂停菜单选择索引 0:继续 1:重新开始 2:返回主菜单
    bool escKeyReleased = true;          // ESC键防重复触发
    int resultMenuSelection = 0;         // 结算菜单选择索引 0:重新开始 1:返回菜单

    // ========== 鼠标点击支持 ==========
    bool mouseWasPressed = false;        // 鼠标左键上一帧状态（防重复触发）
    bool isMouseClick();                 // 检测鼠标单击（按下瞬间返回true）
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh); // 点是否在矩形内

    // ========== V3.1: 音量调节 ==========
    float musicVolume = 1.0f;            // 背景音乐音量 0.0~1.0
    float effectVolume = 1.0f;           // 音效音量 0.0~1.0
    int settingsMenuSelection = 0;       // 设置菜单选择索引
    bool isInSettings = false;            // 是否在设置界面
    bool isAdjustingVolume = false;       // 是否正在调节音量
    std::string configFilePath;           // 配置文件完整路径

    // ========== V3.1: 按键自定义 ==========
    int customKeys[6] = {  // 自定义按键，存储虚拟键码，默认A/S/D/F/J/K
        'A', 'S', 'D', 'F', 'J', 'K'
    };
    int currentKeySettingIndex = -1; // 正在设置的按键索引，-1表示未在设置
    bool keySettingWaitingRelease = false; // 等待按键松开标志
    const char* getKeyName(int vkCode); // 虚拟键码转显示名称

    // 谱面数据
    std::vector<std::pair<long long, int>> noteTimeData;
    long long lastNoteTime;

    static const int TRACK_COUNT = 6;
    static const int TRACK_WIDTH = 70;
    static const int JUDGE_Y = 500;
    static const char TRACK_KEYS[6];
    static const COLORREF TRACK_COLORS[6];

    // 难度对应的下落速度
    static const double DIFFICULTY_SPEEDS[3];

    void initTracks();
    void loadDemoSong();
    bool loadSongFromMP3(const std::string& filePath);
    void handleInput();
    void handleResultInput();
    void handleMenuInput();
    void handleTrackDelegateInput();
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
    void drawBPMIndicator();     // V3.0: BPM指示器
    void drawNotePreviewBar();   // V3.0: 音符预读条
    void drawParticles();        // V3.0: 判定粒子特效
    void drawProgressBar();      // V3.0: 歌曲进度条
    void spawnParticles(float x, float y, Judgement j);  // V3.0: 生成粒子
    void updateParticles(float dt);   // V3.0: 更新粒子
    void updateKeyFeedback(float dt); // V3.0: 更新按键反馈
    // ========== V3.1 新增方法 ==========
    void drawPauseScreen();      // 暂停界面
    void drawSettingsScreen();   // 设置界面
    void drawNewResultScreen();  // 新结算界面
    void handlePauseInput();     // 暂停界面输入处理
    void handleSettingsInput();  // 设置界面输入处理
    void handleAnalysisInput();  // V3.2: 分析界面输入处理
    void drawAnalysisScreen();   // V3.2: 分析界面绘制
    void drawAchievementsScreen(); // V3.3: 成就界面
    void handleAchievementsInput(); // V3.3: 成就界面输入
    void drawSongListScreen();     // V3.4: 歌曲列表界面
    void drawTrackDelegateScreen();// 代管轨道选择界面
    void handleSongListInput();    // V3.4: 歌曲列表输入
    void drawTutorialScreen();     // 游玩说明界面
    void handleTutorialInput();    // 游玩说明输入
    void drawAchievementPopups(); // V3.3: 成就弹窗绘制
    void updateAchievementPopups(float dt);
    std::string getFileNameWithoutExt(const std::string& filePath);
    void loadSongForPlaying();   // V3.2: 加载歌曲进入游戏
    void startDelegatedGame();   // 代管确认后启动游戏
    void loadConfig(const std::string& path = "config.ini"); // 加载本地配置
    void saveConfig();           // 保存本地配置
    void startAnalysis();        // V3.2: 开始分析歌曲
    void exportCurrentSong();    // V3.3: 导出当前歌曲
    void importChartPackage();   // V3.3: 导入谱面
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
