/**
 * @file AchievementSystem.cpp
 * @brief 成就系统实现
 */
#include "AchievementSystem.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <algorithm>

AchievementSystem::AchievementSystem() {
    // 初始化10个成就
    achievements = {
        {1,  "初出茅庐",   "完成第一首歌",                 "🎵"},
        {2,  "连击达人",   "首次达成50 Combo",              "🔥"},
        {3,  "完美演奏",   "首次Full Combo（无Miss）",      "⭐"},
        {4,  "绝对音准",   "首次All Perfect（全Perfect）",  "💎"},
        {5,  "百斩",       "累计完成100首歌",               "💯"},
        {6,  "千分",       "单首得分超过100000分",           "🏆"},
        {7,  "硬核玩家",   "完成任意困难难度歌曲",           "💪"},
        {8,  "慢工出细活", "0.5倍速完成一首歌",             "🐢"},
        {9,  "闪电手",     "2倍速完成一首歌",               "⚡"},
        {10, "收藏家",     "导入超过10首自定义谱面",         "📦"},
    };
}

void AchievementSystem::init(const std::string& savePath) {
    saveFilePath = savePath;
    loadSave();
}

void AchievementSystem::loadSave() {
    FILE* fp = fopen(saveFilePath.c_str(), "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        int id = 0;
        char timeStr[64] = "";
        // 格式: id|unlockTime
        if (sscanf(line, "%d|%63[^\n]", &id, timeStr) >= 1) {
            for (auto& a : achievements) {
                if (a.id == id) {
                    a.unlocked = true;
                    a.unlockedTime = timeStr;
                    break;
                }
            }
        }
    }
    fclose(fp);
    printf("[Achievement] 加载成就存档: %d/%zu 已解锁\n", getUnlockedCount(), achievements.size());
    fflush(stdout);
}

void AchievementSystem::save() {
    FILE* fp = fopen(saveFilePath.c_str(), "w");
    if (!fp) return;

    fprintf(fp, "# BeatPixel 成就存档\n");
    for (const auto& a : achievements) {
        if (a.unlocked) {
            fprintf(fp, "%d|%s\n", a.id, a.unlockedTime.c_str());
        }
    }
    fclose(fp);
}

int AchievementSystem::getUnlockedCount() const {
    int count = 0;
    for (const auto& a : achievements) {
        if (a.unlocked) count++;
    }
    return count;
}

void AchievementSystem::unlock(int id) {
    for (auto& a : achievements) {
        if (a.id == id && !a.unlocked) {
            a.unlocked = true;
            // 记录解锁时间
            time_t now = time(nullptr);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localtime(&now));
            a.unlockedTime = timeStr;
            printf("[Achievement] 解锁: %s - %s\n", a.icon, a.name);
            fflush(stdout);
            break;
        }
    }
}

std::vector<int> AchievementSystem::checkAchievements(
    const std::string& songName,
    int difficulty,
    int score,
    int maxCombo,
    int perfectCount,
    int goodCount,
    int missCount,
    int totalSongsPlayed,
    int customSongCount,
    float playbackSpeed) {

    std::vector<int> newlyUnlocked;
    int total = perfectCount + goodCount + missCount;

    // 1. 初出茅庐：完成第一首歌
    if (total > 0 && !achievements[0].unlocked) {
        unlock(1);
        newlyUnlocked.push_back(1);
    }

    // 2. 连击达人：首次达成50 Combo
    if (maxCombo >= 50 && !achievements[1].unlocked) {
        unlock(2);
        newlyUnlocked.push_back(2);
    }

    // 3. 完美演奏：首次Full Combo（无Miss）
    if (missCount == 0 && total > 0 && !achievements[2].unlocked) {
        unlock(3);
        newlyUnlocked.push_back(3);
    }

    // 4. 绝对音准：首次All Perfect
    if (goodCount == 0 && missCount == 0 && total > 0 && !achievements[3].unlocked) {
        unlock(4);
        newlyUnlocked.push_back(4);
    }

    // 5. 百斩：累计完成100首歌
    if (totalSongsPlayed >= 100 && !achievements[4].unlocked) {
        unlock(5);
        newlyUnlocked.push_back(5);
    }

    // 6. 千分：单首得分超过100000
    if (score >= 100000 && !achievements[5].unlocked) {
        unlock(6);
        newlyUnlocked.push_back(6);
    }

    // 7. 硬核玩家：完成困难难度
    if (difficulty == 2 && total > 0 && !achievements[6].unlocked) {
        unlock(7);
        newlyUnlocked.push_back(7);
    }

    // 8. 慢工出细活：0.5倍速完成
    if (playbackSpeed <= 0.5f && total > 0 && !achievements[7].unlocked) {
        unlock(8);
        newlyUnlocked.push_back(8);
    }

    // 9. 闪电手：2倍速完成
    if (playbackSpeed >= 2.0f && total > 0 && !achievements[8].unlocked) {
        unlock(9);
        newlyUnlocked.push_back(9);
    }

    // 10. 收藏家：导入超过10首自定义谱面
    if (customSongCount >= 10 && !achievements[9].unlocked) {
        unlock(10);
        newlyUnlocked.push_back(10);
    }

    if (!newlyUnlocked.empty()) {
        save();
    }

    return newlyUnlocked;
}
