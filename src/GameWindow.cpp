/**
 * @file GameWindow.cpp
 * @brief GameWindow游戏窗口类实现
 * @details 实现游戏主循环：初始化 -> 60FPS帧循环(输入->更新->渲染) -> 结束
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
      gameStartTime(0), currentTime(0), isRunning(false),
      lastJudgement(NONE), judgementDisplayTimer(0) {
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
 * @details 为每条轨道创建NoteTrack实例，设置独立的轨道参数
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
    // 演示谱面：预设音符时间点(ms)和对应轨道
    // 节奏设计：从慢到快，覆盖4条轨道，模拟真实音游体验
    struct NoteData {
        long long time; // 判定时间戳(ms)，相对于游戏开始
        int track;      // 轨道编号 0-3
    };

    // 基础节奏型：简单交替，让玩家熟悉操作
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

        // 高潮段 (间隔400ms，连续交替)
        {25000, 0}, {25400, 1}, {25800, 2}, {26200, 3},
        {26600, 3}, {27000, 2}, {27400, 1}, {27800, 0},
        {28200, 0}, {28600, 2}, {29000, 1}, {29400, 3},

        // 结尾段 (间隔600ms，回归平稳)
        {30000, 0}, {30600, 1}, {31200, 2}, {31800, 3},
        {32400, 1}, {33000, 2}, {33600, 0}, {34200, 3},
    };

    int noteCount = sizeof(demoNotes) / sizeof(demoNotes[0]);

    for (int i = 0; i < noteCount; i++) {
        auto note = std::make_unique<NormalNote>(
            demoNotes[i].track,
            demoNotes[i].time,  // 判定时间戳
            JUDGE_Y,
            NOTE_SPEED,
            TRACK_COLORS[demoNotes[i].track]
        );
        tracks[demoNotes[i].track]->addNote(std::move(note));
    }
}

/**
 * @brief 初始化游戏
 * @details 创建图形窗口、初始化轨道、加载谱面、记录开始时间
 * @return true成功，false失败
 */
bool GameWindow::init() {
    // 创建EasyX图形窗口
    initgraph(width, height);
    setbkcolor(RGB(15, 15, 15)); // 深色背景
    cleardevice();

    // 初始化4条轨道
    initTracks();

    // 加载演示谱面
    loadDemoSong();

    // 记录游戏开始时间
    gameStartTime = GetTickCount64();
    currentTime = 0;
    isRunning = true;

    return true;
}

/**
 * @brief 处理按键输入
 * @details 检测A/S/D/F按键，触发对应轨道的判定逻辑
 *          使用EasyX的GetAsyncKeyState异步检测，支持多键同时按下
 */
void GameWindow::handleInput() {
    // 检测4个轨道按键 (A=0x41, S=0x53, D=0x44, F=0x46)
    for (int i = 0; i < TRACK_COUNT; i++) {
        // 使用GetAsyncKeyState检测按键状态
        // 需要将char转为对应的虚拟键码
        SHORT keyState = GetAsyncKeyState(TRACK_KEYS[i]);
        if (keyState & 0x8000) { // 最高位为1表示按键被按下
            Judgement result = tracks[i]->handlePress(currentTime);
            if (result != NONE) {
                scoreSystem.addJudgement(result);
                lastJudgement = result;
                judgementDisplayTimer = 30; // 显示30帧(0.5秒)
            }
        }
    }

    // ESC键退出
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        isRunning = false;
    }
}

/**
 * @brief 更新游戏状态
 * @details 更新当前时间，驱动所有轨道的音符位置更新
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
}

/**
 * @brief 渲染画面
 * @details 清屏 -> 绘制轨道和音符 -> 绘制UI -> 刷新显示
 */
void GameWindow::render() {
    // 双缓冲开始，防止画面闪烁
    BeginBatchDraw();
    cleardevice();

    // 绘制所有轨道（含轨道内音符）
    for (auto& track : tracks) {
        track->draw();
    }

    // 绘制UI界面
    drawUI();

    // 双缓冲结束，显示画面
    FlushBatchDraw();
}

/**
 * @brief 绘制UI界面
 * @details 显示：当前分数、Combo数、最近判定结果、操作提示
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
        // Combo数越大字体越大，增强视觉冲击
        int comboSize = 28 + std::min(scoreSystem.getCurrentCombo() / 10, 5) * 4;
        settextstyle(comboSize, 0, "Consolas");

        // Combo数超过50变为金色，超过100变为红色
        COLORREF comboColor = RGB(255, 255, 255);
        if (scoreSystem.getCurrentCombo() >= 100) {
            comboColor = RGB(255, 50, 50);
        } else if (scoreSystem.getCurrentCombo() >= 50) {
            comboColor = RGB(255, 215, 0);
        }
        settextcolor(comboColor);

        char comboStr[32];
        snprintf(comboStr, sizeof(comboStr), "%d COMBO", scoreSystem.getCurrentCombo());
        // 右对齐显示
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
                judgeColor = RGB(255, 215, 0); // 金色
                break;
            case GOOD:
                judgeStr = "GOOD";
                judgeColor = RGB(100, 255, 100); // 绿色
                break;
            case MISS:
                judgeStr = "MISS";
                judgeColor = RGB(255, 80, 80); // 红色
                break;
            default:
                break;
        }

        // 判定文字带缩放动画效果（从大到小）
        float scale = 1.0f + 0.3f * ((float)judgementDisplayTimer / 30.0f);
        int judgeSize = (int)(32 * scale);
        settextstyle(judgeSize, 0, "Consolas");
        settextcolor(judgeColor);
        int judgeW = textwidth(judgeStr);
        // 在判定线上方显示
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
 * @brief 游戏主循环
 * @details 以60FPS执行：处理输入 -> 更新状态 -> 渲染画面
 *          使用精确帧率控制，确保游戏运行流畅
 */
void GameWindow::run() {
    const int FRAME_TIME = 1000 / fps; // 每帧目标时间(ms)

    while (isRunning) {
        long long frameStart = GetTickCount64();

        // 1. 处理输入
        handleInput();

        // 2. 更新游戏状态
        update();

        // 3. 渲染画面
        render();

        // 4. 帧率控制：补足剩余时间，确保稳定60FPS
        long long elapsed = GetTickCount64() - frameStart;
        if (elapsed < FRAME_TIME) {
            Sleep((DWORD)(FRAME_TIME - elapsed));
        }
    }

    // 游戏结束，关闭图形窗口
    closegraph();
}
