/**
 * @file DataManager.cpp
 * @brief 数据存储类实现 - V2.0新增
 * @details 使用简单的文本文件存储排行榜数据
 *          文件格式：每行一条记录，字段用 | 分隔
 *          格式：歌曲名|难度|最高分|最高Combo|游玩次数|最后游玩时间
 */
#include "DataManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstdio>

DataManager::DataManager() {
}

bool DataManager::init(const std::string& dataPath) {
    filePath = dataPath;
    return loadFromFile();
}

/**
 * @brief 从文件加载排行榜数据
 * @details 如果文件不存在，创建空文件
 */
bool DataManager::loadFromFile() {
    entries.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        // 文件不存在，尝试创建
        std::ofstream createFile(filePath);
        if (createFile.is_open()) {
            createFile << "# 《像素节拍》排行榜数据\n";
            createFile.close();
            printf("[DataManager] 创建新的排行榜文件: %s\n", filePath.c_str());
            fflush(stdout);
            return true;
        }
        fprintf(stderr, "[DataManager] 无法创建文件: %s\n", filePath.c_str());
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') continue;

        try {
            std::istringstream iss(line);
            std::string token;
            LeaderboardEntry entry;

            // 解析字段：歌曲名|难度|最高分|最高Combo|游玩次数|最后游玩时间
            if (std::getline(iss, entry.songName, '|') &&
                std::getline(iss, token, '|') &&
                std::getline(iss, token, '|')) {
                entry.difficulty = std::stoi(token);
                if (std::getline(iss, token, '|')) {
                    entry.highScore = std::stoi(token);
                    if (std::getline(iss, token, '|')) {
                        entry.maxCombo = std::stoi(token);
                        if (std::getline(iss, token, '|')) {
                            entry.playCount = std::stoi(token);
                            std::getline(iss, entry.lastPlayed, '|');
                            entries.push_back(entry);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            // 跳过格式错误的行，避免崩溃
            fprintf(stderr, "[DataManager] 跳过无效行: %s (%s)\n", line.c_str(), e.what());
        }
    }

    file.close();
    printf("[DataManager] 加载 %zu 条排行榜记录\n", entries.size());
    fflush(stdout);
    return true;
}

/**
 * @brief 将排行榜数据保存到文件
 */
bool DataManager::saveToFile() {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        fprintf(stderr, "[DataManager] 无法写入文件: %s\n", filePath.c_str());
        return false;
    }

    file << "# 《像素节拍》排行榜数据\n";
    for (const auto& entry : entries) {
        file << entry.songName << "|"
             << entry.difficulty << "|"
             << entry.highScore << "|"
             << entry.maxCombo << "|"
             << entry.playCount << "|"
             << entry.lastPlayed << "|\n";
    }

    file.close();
    return true;
}

/**
 * @brief 生成记录的唯一键（歌曲名+难度）
 */
std::string DataManager::makeKey(const std::string& songName, int difficulty) const {
    return songName + "_" + std::to_string(difficulty);
}

/**
 * @brief 保存游戏结果
 * @details 如果是新记录，直接添加；如果已存在，更新最高分和最高Combo
 * @return 是否刷新了最高分记录
 */
bool DataManager::saveResult(const std::string& songName, int difficulty,
                              int score, int combo) {
    // 获取当前时间
    time_t now = time(nullptr);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localtime(&now));

    bool isNewHighScore = false;

    // 查找已有记录
    auto it = std::find_if(entries.begin(), entries.end(),
        [&](const LeaderboardEntry& e) {
            return e.songName == songName && e.difficulty == difficulty;
        });

    if (it != entries.end()) {
        // 更新已有记录
        if (score > it->highScore) {
            it->highScore = score;
            isNewHighScore = true;
        }
        if (combo > it->maxCombo) {
            it->maxCombo = combo;
        }
        it->playCount++;
        it->lastPlayed = timeStr;
    } else {
        // 新增记录
        LeaderboardEntry entry;
        entry.songName = songName;
        entry.difficulty = difficulty;
        entry.highScore = score;
        entry.maxCombo = combo;
        entry.playCount = 1;
        entry.lastPlayed = timeStr;
        entries.push_back(entry);
        isNewHighScore = true;
    }

    // 保存到文件
    saveToFile();

    if (isNewHighScore) {
        printf("[DataManager] 新纪录! %s 难度%d 分数:%d Combo:%d\n",
               songName.c_str(), difficulty, score, combo);
        fflush(stdout);
    }

    return isNewHighScore;
}

int DataManager::getHighScore(const std::string& songName, int difficulty) const {
    auto it = std::find_if(entries.begin(), entries.end(),
        [&](const LeaderboardEntry& e) {
            return e.songName == songName && e.difficulty == difficulty;
        });
    return (it != entries.end()) ? it->highScore : 0;
}

int DataManager::getMaxCombo(const std::string& songName, int difficulty) const {
    auto it = std::find_if(entries.begin(), entries.end(),
        [&](const LeaderboardEntry& e) {
            return e.songName == songName && e.difficulty == difficulty;
        });
    return (it != entries.end()) ? it->maxCombo : 0;
}

std::vector<LeaderboardEntry> DataManager::getLeaderboard(
    const std::string& songName, int difficulty, int limit) const {

    std::vector<LeaderboardEntry> filtered;

    for (const auto& entry : entries) {
        bool match = true;
        if (!songName.empty() && entry.songName != songName) match = false;
        if (difficulty >= 0 && entry.difficulty != difficulty) match = false;
        if (match) filtered.push_back(entry);
    }

    // 按分数降序排序
    std::sort(filtered.begin(), filtered.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.highScore > b.highScore;
        });

    // 限制返回数量
    if ((int)filtered.size() > limit) {
        filtered.resize(limit);
    }

    return filtered;
}

int DataManager::getTotalPlayCount() const {
    int total = 0;
    for (const auto& entry : entries) {
        total += entry.playCount;
    }
    return total;
}

bool DataManager::hasRecord(const std::string& songName, int difficulty) const {
    return std::any_of(entries.begin(), entries.end(),
        [&](const LeaderboardEntry& e) {
            return e.songName == songName && e.difficulty == difficulty;
        });
}
