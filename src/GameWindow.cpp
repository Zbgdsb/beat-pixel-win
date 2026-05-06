/**
 * @file GameWindow.cpp
 * @brief GameWindow游戏窗口类实现
 * @details 实现游戏主循环：初始化 -> 60FPS帧循环(输入->更新->渲染) -> 结算界面 -> 结束
 *          集成音频管理器，支持背景音乐播放和按键音效
 */
#include "GameWindow.h"
#include <ctime>
#include <cstdio>
#include <cmath>

// 静态常量定义
const double GameWindow::NOTE_SPEED = 5.0; // 5px/帧 ≈ 300px/s @60FPS
const char GameWindow::TRACK_KEYS[4] = {'A', 'S', 'D', 'F'};
const COLORREF GameWindow::TRACK_COLORS[4] = {
    RGB(0, 255, 255),   // 青色 - 轨道0 (A键)
    RGB(255, 100, 100), // 红色 - 轨道1 (S键)
    RGB(100, 255, 100), // 绿色 - 轨道2 (D键)
    RGB(255, 255, 100)  // 黄色 - 轨道3 (F键)
};

/**
 * @brief 构造函数
 * @param width  窗口宽度，默认800px
 * @param height 窗口高度，默认650px
 * @param fps    目标帧率，默认60
 */
GameWindow::GameWindow(int width, int height, int fps)
    : width(width), height(height), fps(fps),
      gameState(PLAYING), gameStartTime(0), currentTime(0), isRunning(false),
      lastJudgement(NONE), judgementDisplayTimer(0), resultStartTime(0) {
}

/**
 * @brief 计算轨道左上角X坐标
 * @details 4条轨道居中排列，每条轨道宽度TRACK_WIDTH，间隔10px
 * @param trackId 轨道编号 0-3
 * @return 轨道左上角X坐标
 */
int GameWindow::getTrackX(int trackId) const {
    int totalWidth = TRACK_COUNT * TRACK_WIDTH + (TRACK_COUNT - 1) * 10;
    int startX = (width - totalWidth) / 2;
    return startX + trackId * (TRACK_WIDTH + 10);
}

/**
 * @brief 初始化4条轨道
 */
void GameWindow::initTracks() {
    for (int i = 0; i < TRACK_COUNT; i++) {
        int x = getTrackX(i);
        auto track = std::make_unique<NoteTrack>(
            i, x, TRACK_WIDTH, JUDGE_Y, NOTE_SPEED, TRACK_KEYS[i]
        );
        tracks.push_back(std::move(track));
    }
}

/**
 * @brief 加载演示谱面
 * @details 生成一组预设的音符时间点，模拟一首歌的节奏
 *          音符间隔从800ms到400ms逐步加快，增加游戏趣味性
 */
void GameWindow::loadDemoSong() {
    struct NoteData {
        long long time;
        int track;
    };

    NoteData demoNotes[] = {
        // 开场慢速段 (间隔1000ms)
        {2000, 0}, {3000, 1}, {4000, 2}, {5000, 3},
        {6000, 0}, {7000, 1}, {8000, 2}, {9000, 3},
        // 中速段 (间隔800ms)
        {10000, 0}, {10800, 2}, {11600, 1}, {12400, 3},
        {13200, 0}, {14000, 2}, {14800, 1}, {15600, 3},
        // 双押段 (同时按下两个键)
        {16500, 0}, {16500, 3}, {17500, 1}, {17500, 2},
        {18500, 0}, {18500, 3}, {19500, 1}, {19500, 2},
        // 快速段 (间隔500ms)
        {20500, 0}, {21000, 1}, {21500, 2}, {22000, 3},
        {22500, 0}, {23000, 1}, {23500, 2}, {24000, 3},
        // 高潮段 (间隔400ms)
        {25000, 0}, {25400, 1}, {25800, 2}, {26200, 3},
        {26600, 3}, {27000, 2}, {27400, 1}, {27800, 0},
        {28200, 0}, {28600, 2}, {29000, 1}, {29400, 3},
        // 结尾段 (间隔600ms)
        {30000, 0}, {30600, 1}, {31200, 2}, {31800, 3},
        {32400, 1}, {33000, 2}, {33600, 0}, {34200, 3},
    };

    int noteCount = sizeof(demoNotes) / sizeof(demoNotes[0]);

    for (int i = 0; i < noteCount; i++) {
        auto note = std::make_unique<NormalNote>(
            demoNotes[i].track,
            demoNotes[i].time,
            JUDGE_Y,
            NOTE_SPEED,
            TRACK_COLORS[demoNotes[i].track]
        );
        tracks[demoNotes[i].track]->addNote(std::move(note));
    }
}

/**
 * @brief 检测所有音符是否已处理完毕
 * @details 所有轨道都没有未判定的音符时返回true
 */
bool GameWindow::isAllNotesCleared() const {
    for (const auto& track : tracks) {
        if (!track->isEmpty()) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 根据Perfect率计算评级
 * @return S/A/B/C/D 评级字符串
 */
const char* GameWindow::getRating() const {
    int total = scoreSystem.getPerfectCount() + scoreSystem.getGoodCount() + scoreSystem.getMissCount();
    if (total == 0) return "D";

    float perfectRate = (float)scoreSystem.getPerfectCount() / total;
    if (perfectRate >= 0.95f) return "S";
    if (perfectRate >= 0.80f) return "A";
    if (perfectRate >= 0.60f) return "B";
    if (perfectRate >= 0.40f) return "C";
    return "D";
}

/**
 * @brief 获取评级对应颜色
 * @details S=金色, A=青色, B=绿色, C=黄色, D=灰色
 */
COLORREF GameWindow::getRatingColor() const {
    const char* rating = getRating();
    switch (rating[0]) {
        case 'S': return RGB(255, 215, 0);   // 金色
        case 'A': return RGB(0, 255, 255);   // 青色
        case 'B': return RGB(100, 255, 100); // 绿色
        case 'C': return RGB(255, 255, 100); // 黄色
        default:  return RGB(150, 150, 150); // 灰色
    }
}

/**
 * @brief 初始化游戏
 * @details 创建图形窗口、初始化轨道、音频和计分系统，加载谱面
 */
bool GameWindow::init() {
    // 创建EasyX图形窗口
    initgraph(width, height);
    setbkcolor(RGB(15, 15, 15));
    cleardevice();

    // 初始化4条轨道
    initTracks();

    // 加载演示谱面
    loadDemoSong();

    // 初始化音频系统（失败不阻塞游戏）
    audioManager.init();

    // 记录游戏开始时间
    gameStartTime = GetTickCount64();
    currentTime = 0;
    isRunning = true;
    gameState = PLAYING;

    return true;
}

/**
 * @brief 处理按键输入（游戏中状态）
 * @details 检测A/S/D/F按键，触发对应轨道的判定逻辑
 */
void GameWindow::handleInput() {
    // 处理SFML窗口事件（关闭等）
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    // 检测4个轨道按键
    for (int i = 0; i < TRACK_COUNT; i++) {
        SHORT keyState = GetAsyncKeyState(TRACK_KEYS[i]);
        if (keyState & 0x8000) {
            Judgement result = tracks[i]->handlePress(currentTime);
            if (result != NONE) {
                scoreSystem.addJudgement(result);
                lastJudgement = result;
                judgementDisplayTimer = 30;

                // 播放对应音效
                if (result == PERFECT) {
                    audioManager.playHit(true);
                } else if (result == GOOD) {
                    audioManager.playHit(false);
                } else if (result == MISS) {
                    audioManager.playMiss();
                }
            }
        }
    }

    // ESC键退出
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        isRunning = false;
    }
}

/**
 * @brief 处理按键输入（结算界面状态）
 * @details Enter键重新开始，ESC键退出
 */
void GameWindow::handleResultInput() {
    // 处理SFML窗口事件（关闭等）
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    // Enter键：重新开始游戏
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        // 重置所有状态
        scoreSystem.reset();
        tracks.clear();
        initTracks();
        loadDemoSong();
        gameStartTime = GetTickCount64();
        currentTime = 0;
        lastJudgement = NONE;
        judgementDisplayTimer = 0;
        gameState = PLAYING;
        audioManager.playBGM();
    }

    // ESC键退出
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        isRunning = false;
    }
}

/**
 * @brief 更新游戏状态
 */
void GameWindow::update() {
    // 更新游戏内时间
    currentTime = GetTickCount64() - gameStartTime;

    // 更新所有轨道的音符状态
    for (auto& track : tracks) {
        track->update(currentTime, scoreSystem);
    }

    // 更新判定显示计时器
    if (judgementDisplayTimer > 0) {
        judgementDisplayTimer--;
        if (judgementDisplayTimer == 0) {
            lastJudgement = NONE;
        }
    }

    // 检测游戏是否结束（所有音符已处理完毕）
    if (isAllNotesCleared()) {
        audioManager.stopBGM();
        gameState = RESULT;
        resultStartTime = GetTickCount64();
    }
}

/**
 * @brief 渲染画面（根据状态分发）
 */
void GameWindow::render() {
    BeginBatchDraw();
    cleardevice();

    if (gameState == PLAYING) {
        // 绘制所有轨道（含轨道内音符）
        for (auto& track : tracks) {
            track->draw();
        }
        // 绘制UI界面
        drawUI();
    } else if (gameState == RESULT) {
        // 绘制结算界面
        drawResultScreen();
    }

    FlushBatchDraw();
}

/**
 * @brief 绘制UI界面（游戏中状态）
 */
void GameWindow::drawUI() {
    // ---- 分数显示（左上角） ----
    settextcolor(RGB(255, 255, 255));
    settextstyle(24, 0, "Consolas");
    char scoreStr[64];
    snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d", scoreSystem.getTotalScore());
    outtextxy(20, 20, scoreStr);

    // ---- Combo显示（右上角） ----
    if (scoreSystem.getCurrentCombo() > 0) {
        int comboSize = 28 + std::min(scoreSystem.getCurrentCombo() / 10, 5) * 4;
        settextstyle(comboSize, 0, "Consolas");

        COLORREF comboColor = RGB(255, 255, 255);
        if (scoreSystem.getCurrentCombo() >= 100) {
            comboColor = RGB(255, 50, 50);
        } else if (scoreSystem.getCurrentCombo() >= 50) {
            comboColor = RGB(255, 215, 0);
        }
        settextcolor(comboColor);

        char comboStr[32];
        snprintf(comboStr, sizeof(comboStr), "%d COMBO", scoreSystem.getCurrentCombo());
        int comboW = textwidth(comboStr);
        outtextxy(width - comboW - 20, 20, comboStr);
    }

    // ---- 判定结果显示（屏幕中央） ----
    if (judgementDisplayTimer > 0) {
        Judgement j = scoreSystem.getLastJudgement();
        const char* judgeStr = "";
        COLORREF judgeColor = RGB(255, 255, 255);

        switch (j) {
            case PERFECT:
                judgeStr = "PERFECT!";
                judgeColor = RGB(255, 215, 0);
                break;
            case GOOD:
                judgeStr = "GOOD";
                judgeColor = RGB(100, 255, 100);
                break;
            case MISS:
                judgeStr = "MISS";
                judgeColor = RGB(255, 80, 80);
                break;
            default:
                break;
        }

        float scale = 1.0f + 0.3f * ((float)judgementDisplayTimer / 30.0f);
        int judgeSize = (int)(32 * scale);
        settextstyle(judgeSize, 0, "Consolas");
        settextcolor(judgeColor);
        int judgeW = textwidth(judgeStr);
        outtextxy((width - judgeW) / 2, JUDGE_Y - 60, judgeStr);
    }

    // ---- 操作提示（底部） ----
    settextcolor(RGB(100, 100, 100));
    settextstyle(14, 0, "Consolas");
    outtextxy(20, height - 30, "ESC: Exit  |  Keys: A S D F");

    // ---- 判定统计（左下角） ----
    settextstyle(14, 0, "Consolas");
    settextcolor(RGB(180, 180, 180));
    char statStr[128];
    snprintf(statStr, sizeof(statStr), "P:%d  G:%d  M:%d  MaxCombo:%d",
             scoreSystem.getPerfectCount(),
             scoreSystem.getGoodCount(),
             scoreSystem.getMissCount(),
             scoreSystem.getMaxCombo());
    outtextxy(20, height - 55, statStr);
}

/**
 * @brief 绘制结算界面
 * @details 显示最终得分、判定统计、评级，带淡入动画效果
 */
void GameWindow::drawResultScreen() {
    // 计算淡入透明度（0.5秒内从0到1）
    float elapsed = (float)(GetTickCount64() - resultStartTime) / 500.0f;
    float alpha = std::min(elapsed, 1.0f);
    (void)alpha; // EasyX无alpha参数，直接绘制

    // ---- 半透明背景遮罩 ----
    setfillcolor(RGB(0, 0, 0));
    fillrectangle(0, 0, width, height);

    // ---- 标题 ----
    settextcolor(RGB(255, 255, 255));
    settextstyle(36, 0, "Consolas");
    const char* title = "SONG COMPLETE!";
    int titleW = textwidth(title);
    outtextxy((width - titleW) / 2, 60, title);

    // ---- 评级（大字居中） ----
    const char* rating = getRating();
    COLORREF ratingColor = getRatingColor();
    settextstyle(80, 0, "Consolas");
    settextcolor(ratingColor);
    int ratingW = textwidth(rating);
    outtextxy((width - ratingW) / 2, 110, rating);

    // ---- 最终得分 ----
    settextstyle(28, 0, "Consolas");
    settextcolor(RGB(255, 255, 255));
    char finalScoreStr[64];
    snprintf(finalScoreStr, sizeof(finalScoreStr), "Final Score: %d", scoreSystem.getTotalScore());
    int scoreW = textwidth(finalScoreStr);
    outtextxy((width - scoreW) / 2, 220, finalScoreStr);

    // ---- 最高Combo ----
    settextstyle(22, 0, "Consolas");
    settextcolor(RGB(255, 215, 0));
    char maxComboStr[32];
    snprintf(maxComboStr, sizeof(maxComboStr), "Max Combo: %d", scoreSystem.getMaxCombo());
    int maxComboW = textwidth(maxComboStr);
    outtextxy((width - maxComboW) / 2, 265, maxComboStr);

    // ---- 判定统计 ----
    settextstyle(20, 0, "Consolas");
    int statsY = 320;
    int lineH = 30;

    // Perfect（金色）
    settextcolor(RGB(255, 215, 0));
    char perfectStr[32];
    snprintf(perfectStr, sizeof(perfectStr), "Perfect:  %d", scoreSystem.getPerfectCount());
    int pW = textwidth(perfectStr);
    outtextxy((width - pW) / 2, statsY, perfectStr);

    // Good（绿色）
    settextcolor(RGB(100, 255, 100));
    char goodStr[32];
    snprintf(goodStr, sizeof(goodStr), "Good:     %d", scoreSystem.getGoodCount());
    int gW = textwidth(goodStr);
    outtextxy((width - gW) / 2, statsY + lineH, goodStr);

    // Miss（红色）
    settextcolor(RGB(255, 80, 80));
    char missStr[32];
    snprintf(missStr, sizeof(missStr), "Miss:     %d", scoreSystem.getMissCount());
    int mW = textwidth(missStr);
    outtextxy((width - mW) / 2, statsY + lineH * 2, missStr);

    // ---- 操作提示 ----
    settextcolor(RGB(120, 120, 120));
    settextstyle(16, 0, "Consolas");
    const char* hint1 = "Press ENTER to Retry";
    const char* hint2 = "Press ESC to Exit";
    int h1W = textwidth(hint1);
    int h2W = textwidth(hint2);
    outtextxy((width - h1W) / 2, height - 80, hint1);
    outtextxy((width - h2W) / 2, height - 55, hint2);
}

/**
 * @brief 游戏主循环
 * @details 以60FPS执行：处理输入 -> 更新状态 -> 渲染画面
 *          游戏结束后自动切换到结算界面
 */
void GameWindow::run() {
    const int FRAME_TIME = 1000 / fps;

    // 开始播放背景音乐
    audioManager.playBGM();

    while (isRunning) {
        long long frameStart = GetTickCount64();

        // 根据游戏状态分发处理
        if (gameState == PLAYING) {
            handleInput();
            if (!isRunning) break;
            update();
        } else if (gameState == RESULT) {
            handleResultInput();
            if (!isRunning) break;
        }

        render();

        // 帧率控制
        long long elapsed = GetTickCount64() - frameStart;
        if (elapsed < FRAME_TIME) {
            Sleep((DWORD)(FRAME_TIME - elapsed));
        }
    }

    // 游戏结束，停止音频并关闭窗口
    audioManager.stopBGM();
    closegraph();
}
