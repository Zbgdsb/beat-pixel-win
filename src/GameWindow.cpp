/**
 * @file GameWindow.cpp
 * @brief GameWindow游戏窗口类实现
 * @details 实现游戏主循环：初始化 -> 60FPS帧循环(输入->更新->渲染) -> 结算界面 -> 结束
 *          集成音频管理器，BGM与谱面节奏同步，按键音效复用Sound对象避免卡顿
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

GameWindow::GameWindow(int width, int height, int fps)
    : width(width), height(height), fps(fps),
      gameState(PLAYING), gameStartTime(0), currentTime(0), isRunning(false),
      lastJudgement(NONE), judgementDisplayTimer(0), resultStartTime(0),
      lastNoteTime(0) {
}

int GameWindow::getTrackX(int trackId) const {
    int totalWidth = TRACK_COUNT * TRACK_WIDTH + (TRACK_COUNT - 1) * 10;
    int startX = (width - totalWidth) / 2;
    return startX + trackId * (TRACK_WIDTH + 10);
}

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
 * @details 生成预设音符时间点，同时记录到noteTimeData供BGM同步使用
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
        // 双押段
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

    // 记录音符时间数据（供BGM同步生成）
    noteTimeData.clear();
    for (int i = 0; i < noteCount; i++) {
        noteTimeData.push_back({demoNotes[i].time, demoNotes[i].track});

        auto note = std::make_unique<NormalNote>(
            demoNotes[i].track,
            demoNotes[i].time,
            JUDGE_Y,
            NOTE_SPEED,
            TRACK_COLORS[demoNotes[i].track]
        );
        tracks[demoNotes[i].track]->addNote(std::move(note));
    }

    // 记录最后一个音符的时间（用于游戏结束判定）
    lastNoteTime = demoNotes[noteCount - 1].time;
}

/**
 * @brief 游戏结束判定
 * @details 当前时间超过最后一个音符时间+2秒，认为游戏结束
 *          比等待音符滚出屏幕更快，避免空档期
 */
bool GameWindow::isGameFinished() const {
    return currentTime > lastNoteTime + 2000;
}

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

COLORREF GameWindow::getRatingColor() const {
    const char* rating = getRating();
    switch (rating[0]) {
        case 'S': return RGB(255, 215, 0);
        case 'A': return RGB(0, 255, 255);
        case 'B': return RGB(100, 255, 100);
        case 'C': return RGB(255, 255, 100);
        default:  return RGB(150, 150, 150);
    }
}

bool GameWindow::init() {
    initgraph(width, height);
    setbkcolor(RGB(15, 15, 15));
    cleardevice();

    initTracks();
    loadDemoSong();

    // 初始化音频系统
    audioManager.init();
    // 生成与谱面节奏同步的BGM
    audioManager.generateSyncedBGM(noteTimeData);

    gameStartTime = GetTickCount64();
    currentTime = 0;
    isRunning = true;
    gameState = PLAYING;

    return true;
}

void GameWindow::handleInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    for (int i = 0; i < TRACK_COUNT; i++) {
        SHORT keyState = GetAsyncKeyState(TRACK_KEYS[i]);
        if (keyState & 0x8000) {
            Judgement result = tracks[i]->handlePress(currentTime);
            if (result != NONE) {
                scoreSystem.addJudgement(result);
                lastJudgement = result;
                judgementDisplayTimer = 30;

                if (result == PERFECT) audioManager.playHit(true);
                else if (result == GOOD) audioManager.playHit(false);
                else if (result == MISS) audioManager.playMiss();
            }
        }
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        isRunning = false;
    }
}

void GameWindow::handleResultInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        scoreSystem.reset();
        tracks.clear();
        initTracks();
        loadDemoSong();
        audioManager.generateSyncedBGM(noteTimeData);
        gameStartTime = GetTickCount64();
        currentTime = 0;
        lastJudgement = NONE;
        judgementDisplayTimer = 0;
        gameState = PLAYING;
        audioManager.playBGM();
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        isRunning = false;
    }
}

void GameWindow::update() {
    currentTime = GetTickCount64() - gameStartTime;

    for (auto& track : tracks) {
        track->update(currentTime, scoreSystem);
    }

    if (judgementDisplayTimer > 0) {
        judgementDisplayTimer--;
        if (judgementDisplayTimer == 0) lastJudgement = NONE;
    }

    // 游戏结束检测：时间到达后自动切换到结算界面
    if (isGameFinished()) {
        audioManager.stopBGM();
        gameState = RESULT;
        resultStartTime = GetTickCount64();
    }
}

void GameWindow::render() {
    BeginBatchDraw();
    cleardevice();

    if (gameState == PLAYING) {
        for (auto& track : tracks) track->draw();
        drawUI();
    } else if (gameState == RESULT) {
        drawResultScreen();
    }

    FlushBatchDraw();
}

void GameWindow::drawUI() {
    // 分数
    settextcolor(RGB(255, 255, 255));
    settextstyle(24, 0, "Consolas");
    char scoreStr[64];
    snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d", scoreSystem.getTotalScore());
    outtextxy(20, 20, scoreStr);

    // Combo
    if (scoreSystem.getCurrentCombo() > 0) {
        int comboSize = 28 + std::min(scoreSystem.getCurrentCombo() / 10, 5) * 4;
        settextstyle(comboSize, 0, "Consolas");
        COLORREF comboColor = RGB(255, 255, 255);
        if (scoreSystem.getCurrentCombo() >= 100) comboColor = RGB(255, 50, 50);
        else if (scoreSystem.getCurrentCombo() >= 50) comboColor = RGB(255, 215, 0);
        settextcolor(comboColor);
        char comboStr[32];
        snprintf(comboStr, sizeof(comboStr), "%d COMBO", scoreSystem.getCurrentCombo());
        int comboW = textwidth(comboStr);
        outtextxy(width - comboW - 20, 20, comboStr);
    }

    // 判定结果
    if (judgementDisplayTimer > 0) {
        Judgement j = scoreSystem.getLastJudgement();
        const char* judgeStr = "";
        COLORREF judgeColor = RGB(255, 255, 255);
        switch (j) {
            case PERFECT: judgeStr = "PERFECT!"; judgeColor = RGB(255, 215, 0); break;
            case GOOD: judgeStr = "GOOD"; judgeColor = RGB(100, 255, 100); break;
            case MISS: judgeStr = "MISS"; judgeColor = RGB(255, 80, 80); break;
            default: break;
        }
        float scale = 1.0f + 0.3f * ((float)judgementDisplayTimer / 30.0f);
        int judgeSize = (int)(32 * scale);
        settextstyle(judgeSize, 0, "Consolas");
        settextcolor(judgeColor);
        int judgeW = textwidth(judgeStr);
        outtextxy((width - judgeW) / 2, JUDGE_Y - 60, judgeStr);
    }

    // 操作提示
    settextcolor(RGB(100, 100, 100));
    settextstyle(14, 0, "Consolas");
    outtextxy(20, height - 30, "ESC: Exit  |  Keys: A S D F");

    // 判定统计
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

void GameWindow::drawResultScreen() {
    setfillcolor(RGB(0, 0, 0));
    fillrectangle(0, 0, width, height);

    // 标题
    settextcolor(RGB(255, 255, 255));
    settextstyle(36, 0, "Consolas");
    const char* title = "SONG COMPLETE!";
    int titleW = textwidth(title);
    outtextxy((width - titleW) / 2, 60, title);

    // 评级
    const char* rating = getRating();
    COLORREF ratingColor = getRatingColor();
    settextstyle(80, 0, "Consolas");
    settextcolor(ratingColor);
    int ratingW = textwidth(rating);
    outtextxy((width - ratingW) / 2, 110, rating);

    // 最终得分
    settextstyle(28, 0, "Consolas");
    settextcolor(RGB(255, 255, 255));
    char finalScoreStr[64];
    snprintf(finalScoreStr, sizeof(finalScoreStr), "Final Score: %d", scoreSystem.getTotalScore());
    int scoreW = textwidth(finalScoreStr);
    outtextxy((width - scoreW) / 2, 220, finalScoreStr);

    // 最高Combo
    settextstyle(22, 0, "Consolas");
    settextcolor(RGB(255, 215, 0));
    char maxComboStr[32];
    snprintf(maxComboStr, sizeof(maxComboStr), "Max Combo: %d", scoreSystem.getMaxCombo());
    int maxComboW = textwidth(maxComboStr);
    outtextxy((width - maxComboW) / 2, 265, maxComboStr);

    // 判定统计
    settextstyle(20, 0, "Consolas");
    int statsY = 320;
    int lineH = 30;

    settextcolor(RGB(255, 215, 0));
    char perfectStr[32];
    snprintf(perfectStr, sizeof(perfectStr), "Perfect:  %d", scoreSystem.getPerfectCount());
    outtextxy((width - textwidth(perfectStr)) / 2, statsY, perfectStr);

    settextcolor(RGB(100, 255, 100));
    char goodStr[32];
    snprintf(goodStr, sizeof(goodStr), "Good:     %d", scoreSystem.getGoodCount());
    outtextxy((width - textwidth(goodStr)) / 2, statsY + lineH, goodStr);

    settextcolor(RGB(255, 80, 80));
    char missStr[32];
    snprintf(missStr, sizeof(missStr), "Miss:     %d", scoreSystem.getMissCount());
    outtextxy((width - textwidth(missStr)) / 2, statsY + lineH * 2, missStr);

    // 操作提示
    settextcolor(RGB(120, 120, 120));
    settextstyle(16, 0, "Consolas");
    const char* hint1 = "Press ENTER to Retry";
    const char* hint2 = "Press ESC to Exit";
    outtextxy((width - textwidth(hint1)) / 2, height - 80, hint1);
    outtextxy((width - textwidth(hint2)) / 2, height - 55, hint2);
}

void GameWindow::run() {
    const int FRAME_TIME = 1000 / fps;
    audioManager.playBGM();

    while (isRunning) {
        long long frameStart = GetTickCount64();

        if (gameState == PLAYING) {
            handleInput();
            if (!isRunning) break;
            update();
        } else if (gameState == RESULT) {
            handleResultInput();
            if (!isRunning) break;
        }

        render();

        long long elapsed = GetTickCount64() - frameStart;
        if (elapsed < FRAME_TIME) {
            Sleep((DWORD)(FRAME_TIME - elapsed));
        }
    }

    audioManager.stopBGM();
    closegraph();
}
