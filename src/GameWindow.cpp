/**
 * @file GameWindow.cpp
 * @brief GameWindow游戏窗口类实现 - V2.0完整版
 * @details V2.0新增：主菜单、MP3导入、难度选择、Combo加成显示、排行榜
 */
#include "GameWindow.h"
#include <ctime>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef _WIN32
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

// 获取可执行文件所在目录（兼容Finder双击启动）
static std::string getExeDir() {
    char buf[1024];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        std::string path(buf);
        auto pos = path.find_last_of('/');
        if (pos != std::string::npos) return path.substr(0, pos);
    }
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd))) return cwd;
    return ".";
}

// 静态常量定义
const char GameWindow::TRACK_KEYS[6] = {'A', 'S', 'D', 'F', 'J', 'K'};
const COLORREF GameWindow::TRACK_COLORS[6] = {
    RGB(0, 255, 255),   // 青色 - 轨道0 (A键)
    RGB(255, 100, 100), // 红色 - 轨道1 (S键)
    RGB(100, 255, 100), // 绿色 - 轨道2 (D键)
    RGB(255, 255, 100), // 黄色 - 轨道3 (F键)
    RGB(255, 150, 50),  // 橙色 - 轨道4 (J键/通鼓)
    RGB(200, 100, 255)  // 紫色 - 轨道5 (K键/Ride)
};

// 难度对应的下落速度（px/帧，60FPS下等效于 px/s ÷ 60）
const double GameWindow::DIFFICULTY_SPEEDS[3] = {
    3.33,   // Easy: 200px/s
    5.0,    // Normal: 300px/s
    6.67    // Hard: 400px/s
};

const float GameWindow::PREVIEW_DURATION = 5.0f;   // 预览未来5秒
const int GameWindow::PREVIEW_HEIGHT = 50;          // 预读条高度
const int GameWindow::PREVIEW_Y = 115;              // 预读条Y坐标（移到HUD下方避免遮挡）

GameWindow::GameWindow(int width, int height, int fps)
    : width(width), height(height), fps(fps),
      currentDifficulty(NORMAL),
      gameState(MENU), gameStartTime(0), currentTime(0),
      isRunning(false),
      lastJudgement(NONE), judgementDisplayTimer(0),
      resultStartTime(0), lastNoteTime(0) {
}

int GameWindow::getTrackX(int trackId) const {
    int totalWidth = TRACK_COUNT * TRACK_WIDTH + (TRACK_COUNT - 1) * 10;
    int startX = (width - totalWidth) / 2;
    return startX + trackId * (TRACK_WIDTH + 10);
}

double GameWindow::getDifficultySpeed() const {
    return DIFFICULTY_SPEEDS[(int)currentDifficulty];
}

const char* GameWindow::getDifficultyName() const {
    switch (currentDifficulty) {
        case EASY:   return "简单";
        case NORMAL: return "普通";
        case HARD:   return "困难";
        default:     return "普通";
    }
}

void GameWindow::initTracks() {
    double speed = getDifficultySpeed();
    for (int i = 0; i < TRACK_COUNT; i++) {
        int x = getTrackX(i);
        auto track = std::make_unique<NoteTrack>(
            i, x, TRACK_WIDTH, JUDGE_Y, speed, TRACK_KEYS[i]
        );
        tracks.push_back(std::move(track));
    }
}

void GameWindow::loadDemoSong() {
    noteTimeData.clear();
    srand((unsigned int)time(nullptr));
    double speed = getDifficultySpeed();
    long long currentTimeMs = 2000;
    long long endTime = 35000;
    int lastTrack = -1;

    // 根据难度调整音符密度
    int minGap, maxGap;
    switch (currentDifficulty) {
        case EASY:
            minGap = 800; maxGap = 1500;
            break;
        case NORMAL:
            minGap = 400; maxGap = 800;
            break;
        case HARD:
            minGap = 250; maxGap = 500;
            break;
    }

    while (currentTimeMs < endTime) {
        int track;
        do { track = rand() % 6; } while (track == lastTrack);
        lastTrack = track;

        noteTimeData.push_back({currentTimeMs, track});
        auto note = std::make_unique<NormalNote>(
            track, currentTimeMs, JUDGE_Y, speed, TRACK_COLORS[track]
        );
        tracks[track]->addNote(std::move(note));

        // Hard难度增加双押概率
        int doubleChance = (currentDifficulty == HARD) ? 25 : 15;
        if (rand() % 100 < doubleChance) {
            int track2;
            do { track2 = rand() % 6; } while (track2 == track);
            noteTimeData.push_back({currentTimeMs, track2});
            auto note2 = std::make_unique<NormalNote>(
                track2, currentTimeMs, JUDGE_Y, speed, TRACK_COLORS[track2]
            );
            tracks[track2]->addNote(std::move(note2));
        }

        int gap = minGap + rand() % (maxGap - minGap + 1);
        currentTimeMs += gap;
    }
    lastNoteTime = noteTimeData.back().first;
    currentSongName = "Demo Song";
    currentBPM = 120.0f;  // Demo默认120 BPM
    bpmPulsePhase = 0.0f;
}

/**
 * @brief 从MP3文件加载歌曲
 * @details 使用BeatParser解析音频文件生成谱面
 */
bool GameWindow::loadSongFromMP3(const std::string& filePath) {
    printf("[GameWindow] 正在导入: %s\n", filePath.c_str());
    fflush(stdout);

    parseResult = beatParser.parse(filePath);

    if (!parseResult.success) {
        fprintf(stderr, "[GameWindow] 导入失败: %s\n", parseResult.errorMessage.c_str());
        fflush(stderr);
        return false;
    }

    // 使用解析结果
    noteTimeData = parseResult.noteTimeData;
    lastNoteTime = parseResult.lastNoteTime;
    currentSongName = filePath;

    // 从路径中提取文件名作为歌曲名
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        currentSongName = filePath.substr(lastSlash + 1);
    }
    // 去掉扩展名
    size_t lastDot = currentSongName.rfind('.');
    if (lastDot != std::string::npos) {
        currentSongName = currentSongName.substr(0, lastDot);
    }

    double speed = getDifficultySpeed();

    // 创建音符对象
    for (const auto& [timeMs, track] : noteTimeData) {
        auto note = std::make_unique<NormalNote>(
            track, timeMs, JUDGE_Y, speed, TRACK_COLORS[track]
        );
        tracks[track]->addNote(std::move(note));
    }

    // 使用解析到的BPM
    currentBPM = parseResult.detectedBPM > 0 ? parseResult.detectedBPM : 120.0f;
    bpmPulsePhase = 0.0f;

    printf("[GameWindow] 歌曲加载完成: %s (%zu个音符, BPM=%.0f)\n",
           currentSongName.c_str(), noteTimeData.size(), currentBPM);
    fflush(stdout);

    return true;
}

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

    // 基于可执行文件位置定位资源目录
    exeDir = getExeDir();
    std::string texPath = exeDir + "/assets/textures";
    std::string configPath = exeDir + "/config.ini";
    std::string lbPath = exeDir + "/leaderboard.dat";
    std::string achPath = exeDir + "/achievements.dat";

    // 加载所有纹理资源
    bool texOk = textures.loadAll(texPath);
    if (!texOk) texOk = textures.loadAll("assets/textures");
    if (!texOk) texOk = textures.loadAll("../assets/textures");
    if (!texOk) {
        printf("[GameWindow] 警告：纹理加载失败，使用后备渲染\n");
    } else {
        printf("[GameWindow] 纹理加载成功\n");
    }
    fflush(stdout);

    // 初始化数据管理器（排行榜）
    dataManager.init(lbPath);

    // 初始化成就系统
    achievementSystem.init(achPath);

    // 加载本地配置（音量等）
    loadConfig(configPath);

    // V3.4: 扫描歌曲列表
    refreshSongList();

    // 保存soundPack，init会重置
    int savedPack = audioManager.getSoundPack();
    audioManager.setResourceDir(exeDir);
    audioManager.init();
    audioManager.setSoundPack(savedPack);
    // 设置初始音量
    audioManager.setMusicVolume(musicVolume);
    audioManager.setEffectVolume(effectVolume);

    isRunning = true;
    gameState = MENU;
    prevCombo = 0;

    return true;
}

// 包含其他分片文件（编译时合并）
#include "GameWindow_input.cpp"
#include "GameWindow_render.cpp"
